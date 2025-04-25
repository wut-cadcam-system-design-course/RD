#include "ml.h"

#include <fstream>
#include <string>
#include <torch/script.h>
#include <torch/torch.h>

static std::vector<std::vector<float>> read_csv(const std::string& p)
{
    std::ifstream f(p);
    if (!f)
        throw std::runtime_error("open " + p);
    std::vector<std::vector<float>> r;
    std::string line;
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string c;
        r.emplace_back();
        while (std::getline(ss, c, ','))
            r.back().push_back(std::stof(c));
    }
    return r;
}
static std::vector<float> read_labels(const std::string& p)
{
    std::ifstream f(p);
    if (!f)
        throw std::runtime_error("open " + p);
    std::vector<float> y;
    std::string l;
    while (std::getline(f, l))
        y.push_back(std::stof(l));
    return y;
}
static std::vector<torch::Tensor> read_obj_vertices(const std::string& p)
{
    std::ifstream f(p);
    if (!f)
        throw std::runtime_error("mesh " + p);
    std::vector<torch::Tensor> v;
    std::string line;
    while (std::getline(f, line))
        if (line.rfind("v ", 0) == 0) {
            std::istringstream ss(line.substr(2));
            float x, y, z;
            ss >> x >> y >> z;
            v.push_back(torch::tensor({ x, y, z }));
        }
    if (v.empty())
        throw std::runtime_error("no vertices");
    return v;
}
static torch::Tensor compute_features(const std::vector<torch::Tensor>& v)
{
    size_t N = v.size();
    auto all = torch::stack(v);
    auto mn = std::get<0>(all.min(0, true)), mx = std::get<0>(all.max(0, true));
    auto span = (mx - mn).clamp(1e-6);
    auto norm = (all - mn) / span;
    torch::Tensor md = torch::zeros({ (long)N });
    for (size_t i = 0; i < N; ++i)
        md[i] = torch::norm(all - all[i], 2, 1).mean();
    return torch::cat({ norm, md.unsqueeze(1) }, 1);
}

// ------------------------------ MODEL ----------------------------------
struct RibNetImpl : torch::nn::Module {
    RibNetImpl(int64_t in_dim = 4, int64_t hid = 64)
    {
        fc1 = register_module("fc1", torch::nn::Linear(in_dim, hid));
        fc2 = register_module("fc2", torch::nn::Linear(hid, hid));
        fc3 = register_module("fc3", torch::nn::Linear(hid, 1));
    }
    torch::Tensor forward(const torch::Tensor& x)
    {
        auto h = torch::relu(fc1->forward(x));
        h = torch::relu(fc2->forward(h));
        return torch::sigmoid(fc3->forward(h));
    }
    torch::nn::Linear fc1 { nullptr }, fc2 { nullptr }, fc3 { nullptr };
};
TORCH_MODULE(RibNet);

// ------------------------------ TRAIN ----------------------------------
static void train(const std::string& fcsv, const std::string& lcsv,
    const std::string& out, int epochs = 30)
{
    auto rows = read_csv(fcsv);
    auto labels = read_labels(lcsv);
    if (rows.size() != labels.size())
        throw std::runtime_error("mismatch");
    const int64_t n = rows.size(), d = rows[0].size();
    torch::Tensor X = torch::empty({ n, d });
    torch::Tensor y = torch::empty({ n, 1 });
    for (int64_t i = 0; i < n; ++i) {
        for (int64_t j = 0; j < d; ++j)
            X[i][j] = rows[i][j];
        y[i][0] = labels[i];
    }
    RibNet net(d);
    net->train();
    torch::optim::Adam opt(net->parameters(), 1e-3);
    for (int e = 0; e < epochs; ++e) {
        auto pred = net->forward(X);
        auto l = torch::binary_cross_entropy(pred, y);
        opt.zero_grad();
        l.backward();
        opt.step();
        std::cout << "Epoch " << e + 1 << "/" << epochs
                  << " BCE=" << l.item().toDouble() << "\n";
    }
    torch::save(net, out);
    std::cout << "Model saved → " << out << "\n";
}

// ------------------------------ INFER -----------------------------------
static void infer(const std::string& obj, const std::string& mdl,
    const std::string& out, float thr = 0.5)
{
    auto v = read_obj_vertices(obj);
    auto feat = compute_features(v);
    RibNet net(4);
    torch::load(net, mdl);
    net->eval();
    torch::NoGradGuard g;
    auto p = net->forward(feat).squeeze();
    auto mask = p > thr;
    std::ofstream f(out);
    if (!f)
        throw std::runtime_error("write");
    for (int64_t i = 0; i < mask.size(0); ++i)
        if (mask[i].item<bool>())
            f << i << "," << std::fixed << std::setprecision(3)
              << p[i].item().toFloat() << "\n";
    std::cout << "Wrote " << out << " (" << mask.sum().item<int64_t>() << ")\n";
}