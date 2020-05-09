/*
 * ftrl.h
 * Copyright (C) 2018 wangxiaoshu <2012wxs@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SRC_OPTIMIZER_FTRL_H_
#define SRC_OPTIMIZER_FTRL_H_

#include <vector>
#include "src/base/base.h"

namespace xflow
{
    int w_dim = 1;
    int v_dim = 10;
    float alpha = 5e-2;
    float beta = 1.0;
    float lambda1 = 5e-5;
    float lambda2 = 10.0;

    class FTRL
    {
        public:
            FTRL() {}
            ~FTRL() {}

            typedef struct FTRLEntry_w
            {
                FTRLEntry_w(int k = w_dim)
                {
                    w.resize(k, 0.0);
                    n.resize(k, 0.0);
                    z.resize(k, 0.0);
                }
                std::vector<float> w;
                std::vector<float> n;
                std::vector<float> z;
            } ftrlentry_w;

            struct KVServerFTRLHandle_w
            {
                void operator()(const ps::KVMeta& req_meta,
                                const ps::KVPairs<float>& req_data,
                                ps::KVServer<float>* server)
                {
                    size_t keys_size = req_data.keys.size();
                    size_t vals_size = req_data.vals.size();
                    ps::KVPairs<float> res;

                    if (req_meta.push)
                    {
                        w_dim = vals_size / keys_size;
                        CHECK_EQ(keys_size, vals_size / w_dim);
                        std::cout << "(9) 收到push的数据，聚合参数w..." << std::endl;
                    }
                    else
                    {
                        res.keys = req_data.keys;
                        res.vals.resize(keys_size * w_dim);
                        std::cout << "(6a) 将参数w写入消息中" << std::endl;
                    }

                    for (size_t i = 0; i < keys_size; ++i)
                    {
                        ps::Key key = req_data.keys[i];
                        FTRLEntry_w& val = store[key];
                        for (int j = 0; j < w_dim; ++j)
                        {
                            if (req_meta.push)
                            {
                                float g = req_data.vals[i * w_dim + j];
                                float old_n = val.n[j];
                                float n = old_n + g * g;
                                val.z[j] += g - (std::sqrt(n) - std::sqrt(old_n)) / alpha * val.w[j];
                                val.n[j] = n;

                                if (std::abs(val.z[j]) <= lambda1)
                                {
                                    val.w[j] = 0.0;
                                }
                                else
                                {
                                    float tmpr = 0.0;
                                    if (val.z[j] > 0.0)
                                        tmpr = val.z[j] - lambda1;
                                    if (val.z[j] < 0.0)
                                        tmpr = val.z[j] + lambda1;
                                    float tmpl = -1 * ((beta + std::sqrt(val.n[j]))/alpha  + lambda2);
                                    val.w[j] = tmpr / tmpl;
                                }
                            }
                            else
                            {
                                res.vals[i * w_dim + j] = val.w[j];
                            }
                        }
                    }

                    if (req_meta.push)
                        std::cout << "(4、10) 回应push消息..." << std::endl;
                    else
                        std::cout << "(6b) 下发模型参数w..." << std::endl;
                    server->Response(req_meta, res);
                }

                private:
                    std::unordered_map<ps::Key, ftrlentry_w> store;
            };

            typedef struct FTRLEntry_v
            {
                FTRLEntry_v(int k = v_dim)
                {
                    w.resize(k, 0.0);
                    n.resize(k, 0.0);
                    z.resize(k, 0.0);
                }
                std::vector<float> w;
                std::vector<float> n;
                std::vector<float> z;
            } ftrlentry_v;

            struct KVServerFTRLHandle_v
            {
                void operator()(const ps::KVMeta& req_meta,
                                const ps::KVPairs<float>& req_data,
                                ps::KVServer<float>* server)
                {
                    size_t keys_size = req_data.keys.size();
                    ps::KVPairs<float> res;

                    if (req_meta.push)
                    {
                        size_t vals_size = req_data.vals.size();
                        CHECK_EQ(keys_size, vals_size / v_dim);
                        std::cout << "(9) 收到push的数据，聚合参数v..." << std::endl;
                    }
                    else
                    {
                        res.keys = req_data.keys;
                        res.vals.resize(keys_size * v_dim);
                        std::cout << "(6a) 将参数v写入消息中" << std::endl;
                    }

                    for (size_t i = 0; i < keys_size; ++i)
                    {
                        ps::Key key = req_data.keys[i];
                        if (store.find(key) == store.end())
                        {
                            FTRLEntry_v val(v_dim);;
                            for (int k = 0; k < v_dim; ++k)
                            {
                                val.w[k] = Base::local_normal_real_distribution<double>(0.0, 1.0)
                                        (Base::local_random_engine()) * 1e-2;
                            }
                            store[key] = val;
                        }

                        FTRLEntry_v& val = store[key];
        
                        for (int j = 0; j < v_dim; ++j)
                        {
                            if (req_meta.push)
                            {
                                float g = req_data.vals[i * v_dim + j];
                                float old_n = val.n[j];
                                float n = old_n + g * g;
                                val.z[j] += g - (std::sqrt(n) - std::sqrt(old_n)) / alpha * val.w[j];
                                val.n[j] = n;

                                if (std::abs(val.z[j]) <= lambda1)
                                {
                                    val.w[j] = 0.0;
                                }
                                else
                                {
                                    float tmpr = 0.0;
                                    if (val.z[j] > 0.0)
                                        tmpr = val.z[j] - lambda1;
                                    if (val.z[j] < 0.0)
                                        tmpr = val.z[j] + lambda1;
                                    float tmpl = -1 * ((beta + std::sqrt(val.n[j]))/alpha  + lambda2);
                                    val.w[j] = tmpr / tmpl;
                                }
                            }
                            else
                            {
                                res.vals[i * v_dim + j] = val.w[j];
                            }
                        }
                    }

                    if (req_meta.push)
                        std::cout << "(10) 回应push消息..." << std::endl;
                    else
                        std::cout << "(6b) 下发模型参数v..." << std::endl;

                    server->Response(req_meta, res);
                }

                private:
                    std::unordered_map<ps::Key, ftrlentry_v> store;
            };

        private:
    };
}  // namespace xflow

#endif  // SRC_OPTIMIZER_FTRL_H_
