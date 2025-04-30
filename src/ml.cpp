#include "ml.h"

#include <torch/torch.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <unordered_set>
#include <stdexcept>
#include <cctype>

static std::vector<std::vector<std::array<float,3>>> compute_knn(
    const std::vector<std::array<float,3>>& verts, size_t k)
{
    size_t N = verts.size();
    std::vector<std::vector<std::pair<float,size_t>>> dists(N);
    // dla każdego wierzchołka i, zbieramy pary (odległość, indeks j)
    for (size_t i = 0; i < N; ++i) {
        dists[i].reserve(N-1);
        for (size_t j = 0; j < N; ++j) {
            if (i == j) continue;
            float dx = verts[j][0] - verts[i][0];
            float dy = verts[j][1] - verts[i][1];
            float dz = verts[j][2] - verts[i][2];
            float dist2 = dx*dx + dy*dy + dz*dz;
            dists[i].emplace_back(dist2, j);
        }
        // wybieramy k najmniejszych odległości
        std::nth_element(dists[i].begin(), dists[i].begin() + std::min(k, dists[i].size()), dists[i].end());
        dists[i].resize(std::min(k, dists[i].size()));
    }
    // budujemy wektory różnic dla każdego wierzchołka
    std::vector<std::vector<std::array<float,3>>> knn_feats(N);
    for (size_t i = 0; i < N; ++i) {
        knn_feats[i].reserve(dists[i].size());
        for (auto& pr : dists[i]) {
            auto const& nbr = verts[pr.second];
            knn_feats[i].push_back({
                nbr[0] - verts[i][0],
                nbr[1] - verts[i][1],
                nbr[2] - verts[i][2]
            });
        }
    }
    return knn_feats;
}

struct OBJParsed {
    std::vector<std::array<float,3>> verts;
    std::vector<int64_t>             label;
};

// proste trim z obu stron
static inline std::string trim(std::string s) {
    const char* ws = " \t\r\n";
    s.erase(0, s.find_first_not_of(ws));
    s.erase(s.find_last_not_of(ws) + 1);
    return s;
}

static OBJParsed parseOBJ(const std::string& path)
{
    std::ifstream f(path, std::ios::in);
    if (!f.is_open())
        throw std::runtime_error("Nie mogę otworzyć pliku: " + path);

    OBJParsed out;
    bool inDoors = false;                     // true → kolejne „v” należą do drzwi
    std::string line;

    while (std::getline(f, line)) {
        if (line.empty()) continue;

        // --------------------------------------------------
        // Deklaracja nowego obiektu                   o ...
        // --------------------------------------------------
        if (line.rfind("o ", 0) == 0) {
            std::string objName = trim(line.substr(2));

            inDoors = objName.find("drzwi");
            continue;
        }

        // --------------------------------------------------
        // Wierzchołki                                    v ...
        // --------------------------------------------------
        if (line.rfind("v ", 0) == 0) {
            std::stringstream ss(line.substr(2));
            float x, y, z;
            ss >> x >> y >> z;

            out.verts.push_back({x, y, z});
            out.label.push_back(inDoors ? 1 : 0);
            continue;
        }

        // --------------------------------------------------
        // Wszystko inne (f, g, usemtl, ... ) pomijamy
        // --------------------------------------------------
    }

    return out;
}

// ==========================================================
// Dataset
// ==========================================================
struct ObjDataset : torch::data::datasets::Dataset<ObjDataset> {
    std::vector<torch::Tensor> data_, targets_;
    size_t n0 = 0, n1 = 0;
    static constexpr size_t K = 8;  // liczba sąsiadów

    explicit ObjDataset(const std::vector<std::string>& files) {
        for (auto& p : files) {
            OBJParsed parsed = parseOBJ(p);
            // obliczamy cechy K-NN
            auto knn = compute_knn(parsed.verts, K);

            for (size_t i = 0; i < parsed.verts.size(); ++i) {
                // zbieramy wektor: [x,y,z, dx1,dy1,dz1, …, dxK,dyK,dzK]
                std::vector<float> feat;
                feat.reserve(3 + 3*K);
                // oryginalne współrzędne
                feat.insert(feat.end(),
                            { parsed.verts[i][0],
                              parsed.verts[i][1],
                              parsed.verts[i][2] });
                // sąsiednie wektory różnic; jeśli mniej niż K, dopełnij zerami
                auto const& neigh = knn[i];
                for (size_t j = 0; j < K; ++j) {
                    if (j < neigh.size()) {
                        feat.insert(feat.end(),
                                    { neigh[j][0], neigh[j][1], neigh[j][2] });
                    } else {
                        feat.insert(feat.end(), {0.f,0.f,0.f});
                    }
                }
                // tworzymy tensor
                auto t = torch::tensor(feat, torch::kFloat32);
                data_.push_back(t);
                int64_t lab = parsed.label[i];
                targets_.push_back(torch::tensor(lab, torch::kInt64));
                (lab==0)? ++n0 : ++n1;
            }
        }
    }

    torch::data::Example<> get(size_t idx) override {
        return { data_[idx], targets_[idx] };
    }
    torch::optional<size_t> size() const override {
        return data_.size();
    }
};

struct DoorNetImpl : torch::nn::Module {
    torch::nn::Linear l1{nullptr}, l2{nullptr}, l3{nullptr};
    static constexpr int64_t IN_DIM = 3 * (ObjDataset::K + 1);
    DoorNetImpl() {
        l1 = register_module("l1", torch::nn::Linear(IN_DIM, 128));
        l2 = register_module("l2", torch::nn::Linear(128, 64));
        l3 = register_module("l3", torch::nn::Linear(64, 2));
    }
    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(l1(x));
        x = torch::relu(l2(x));
        x = l3(x);
        return torch::log_softmax(x,1);
    }
};
TORCH_MODULE(DoorNet);

// ==========================================================
// Trening z wagami klas
// ==========================================================
void train(const std::string& modelPath, const std::vector<std::string>& objs) {
    ObjDataset rawDs(objs);
    auto ds = rawDs.map(torch::data::transforms::Stack<>());
    constexpr int64_t BATCH=64; constexpr size_t EPOCHS=2000;
    auto loader = torch::data::make_data_loader(std::move(ds), BATCH);

    DoorNet net; net->train();

    // Wagi klas =  total / (2 * count_c)
    float total = rawDs.n0 + rawDs.n1;
    float w0 = total / (2.0f*rawDs.n0);
    float w1 = total / (2.0f*rawDs.n1);
    torch::Tensor weights = torch::tensor({w0,w1}, torch::kFloat32);

    torch::optim::AdamW opt(net->parameters(), 1e-3);

    for (size_t e=1;e<=EPOCHS;++e) {
        float lossSum=0; size_t seen=0;
        for (auto& b:*loader) {
            opt.zero_grad();
            auto out=net->forward(b.data);
            auto loss=torch::nll_loss(out, b.target.squeeze(), weights);
            loss.backward(); opt.step();
            lossSum += loss.item<float>() * b.data.size(0); seen += b.data.size(0);
        }
        std::cout<<"Epoch "<<e<<"/"<<EPOCHS<<"  loss="<<lossSum/seen<<"\n";
    }
    torch::save(net, modelPath);
    std::cout<<"Zapisano model: "<<modelPath<<"\n";
}

// ==========================================================
// Zapis OBJ z wynikami
// ==========================================================
void writeOBJ(const std::vector<std::array<float,3>>& verts,
                    const std::unordered_set<size_t>& doors,
                    const std::string& outPath) {
    std::ofstream out(outPath);
    if(!out.is_open()) throw std::runtime_error("Nie mogę zapisać " + outPath);
    // out << "o AllVerts\n";
    // for(const auto& v: verts) out << "v "<<v[0]<<' '<<v[1]<<' '<<v[2]<<"\n";
    out << "o Doors\n";
    for(size_t i=0;i<verts.size();++i) if(doors.count(i)) {
        const auto& v=verts[i]; out << "v "<<v[0]<<' '<<v[1]<<' '<<v[2]<<"\n";
    }
}

// ==========================================================
// Predykcja
// ==========================================================
void predict(const std::string& modelPath,
    const std::string& inOBJ,
    const std::string& outOBJ) {
    DoorNet net; torch::load(net, modelPath); net->eval();

    // 1) Parsujemy verts
    OBJParsed parsed = parseOBJ(inOBJ);
    // 2) Obliczamy cechy KNN dokładnie tak jak w ObjDataset:
    auto knn = compute_knn(parsed.verts, ObjDataset::K);

    std::vector<torch::Tensor> feats;
    feats.reserve(parsed.verts.size());
    for (size_t i = 0; i < parsed.verts.size(); ++i) {
    std::vector<float> f;
    f.reserve(DoorNetImpl::IN_DIM);
    // xyz
    f.push_back(parsed.verts[i][0]);
    f.push_back(parsed.verts[i][1]);
    f.push_back(parsed.verts[i][2]);
    // dx,dy,dz dla K sąsiadów (lub zero)
    for (size_t j = 0; j < ObjDataset::K; ++j) {
    if (j < knn[i].size()) {
        auto &d = knn[i][j];
        f.push_back(d[0]); f.push_back(d[1]); f.push_back(d[2]);
    } else {
        f.push_back(0.f); f.push_back(0.f); f.push_back(0.f);
    }
    }
    feats.push_back(torch::tensor(f, torch::kFloat32));
    }
    auto inputs = torch::stack(feats);

    // 3) Forward + argmax
    std::unordered_set<size_t> doors;
    torch::NoGradGuard no_grad;
    auto log_probs = net->forward(inputs);        // [N×2]
    auto probs     = torch::softmax(log_probs,1); // [N×2]
    auto p_door    = probs.select(1, 1);          // [N] – prawdop. klasy „drzwi”
    
    const float TH = 0.7f;  // dobierz na walidacji tak, by dostać ok. 150 wierzchołków
    for (size_t i = 0; i < p_door.size(0); ++i) {
        if (p_door[i].item<float>() > TH)
            doors.insert(i);
    }

    std::cout << "Wykryto " << doors.size()
        << " / " << parsed.verts.size() << " wierzchołków drzwi.\n";
    writeOBJ(parsed.verts, doors, outOBJ);
    std::cout << "Zapisano: " << outOBJ << "\n";
}