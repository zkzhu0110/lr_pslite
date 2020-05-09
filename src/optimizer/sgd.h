/*
 * sgd.h
 * Copyright (C) 2018 wangxiaoshu <2012wxs@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SRC_OPTIMIZER_SGD_H_
#define SRC_OPTIMIZER_SGD_H_

#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096


namespace xflow
{
    extern int w_dim;
    extern int v_dim;
    float learning_rate = 0.001;

    class SGD
    {
        public:
            SGD() {}
            ~SGD() {}

            typedef struct SGDEntry_w
            {
                SGDEntry_w(int k = w_dim)
                {
                    w.resize(k, 0.0);
                }
                std::vector<float> w;
            } sgdentry_w;

            struct KVServerSGDHandle_w
            {
                void operator()(const ps::KVMeta& req_meta,
                                const ps::KVPairs<float>& req_data,
                                ps::KVServer<float>* server)
                {
                    size_t keys_size = req_data.keys.size();
                    size_t vals_size = req_data.vals.size();
                    ps::KVPairs<float> res;

                    if (req_meta.push)   // worker的push请求
                    {
                        w_dim = vals_size / keys_size;
                        CHECK_EQ(keys_size, vals_size / w_dim);
                        std::cout << "w_dim = " << w_dim << std::endl;
                        std::cout << "(9) 收到push的数据，聚合参数w..." << std::endl;

                        push_num++;

                        for (size_t i = 0; i < keys_size; ++i)
                        {
                            ps::Key key = req_data.keys[i];
                            SGDEntry_w &val = store[key];   // std::unordered_map<ps::Key, sgdentry_w> store;

                            // 全局聚合 
                            for (int j = 0; j < w_dim; ++j)
                            {
                                float g = req_data.vals[i * w_dim + j];
                                val.w[j] -= learning_rate * g;
                            }
                        }

                        std::cout << "(4、10) 回应push消息..." << std::endl;
                        server->Response(req_meta, res);

                        // 已收到并处理好全部的worker数据，发请求给中心域
                        if (push_num == workers_num)
                        {
                            std::cout << "本轮epoch已收到全部worker的数据..." << std::endl;
                            push_num = 0;

                            // 写入数据到文件中
                            char* send_filename = "parameters_store.txt";
                            std::ofstream fout(send_filename);
                            for (size_t i = 0; i < keys_size; ++i)
                            {
                                ps::Key key = req_data.keys[i];
                                SGDEntry_w &val = store[key];   // std::unordered_map<ps::Key, sgdentry_w> store;
                                for (int j = 0; j < w_dim; ++j)
                                    fout << val.w[j] << " ";
                                fout<<std::endl;
                            }
                            fout.close();

                            // TODO: 发送“请求更新”到中心域
                            SendToCenter("Ask for Updating...");

                            // TODO: 发送"请求拉取”到中心域
                            SendToCenter("Ask for Pulling...");

                            // 从中心域中获取文件
                            std::ifstream fin(send_filename);
                            for (size_t i = 0; i < keys_size; ++i)
                            {
                                ps::Key key = req_data.keys[i];
                                SGDEntry_w &val = store[key];   // std::unordered_map<ps::Key, sgdentry_w> store;
                                for (int j = 0; j < w_dim; ++j)
                                    fin >> val.w[j];
                            }
                            fin.close();

                        }
                    }

                    else  // worker的pull请求
                    {
                        std::cout << "(6a) 收到pull请求信息" << std::endl;
                        res.keys = req_data.keys;
                        res.vals.resize(keys_size * w_dim);
                        
                        for (size_t i = 0; i < keys_size; ++i)
                        {
                            ps::Key key = req_data.keys[i];
                            SGDEntry_w &val = store[key];   // std::unordered_map<ps::Key, sgdentry_w> store;

                            // 存入到消息中
                            for (int j = 0; j < w_dim; ++j)
                            {
                                res.vals[i * w_dim + j] = val.w[j];
                            }
                        }

                        std::cout << "(6b) 下发模型参数w..." << std::endl;
                        server->Response(req_meta, res);
                    }
                }

                void SendToCenter(char* my_message)
                {
                    int sockfd, n;
                    char buff[MAXLINE];
                    struct sockaddr_in servaddr;

                    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
                        exit(0);
                    }

                    memset(&servaddr, 0, sizeof(servaddr));
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_port = htons(center_port);
                    if (inet_pton(AF_INET, center_ip, &servaddr.sin_addr) <= 0)
                    {
                        printf("inet_pton error for %s\n", center_ip);
                        return;
                    }

                    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
                    {
                        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
                        return;
                    }

                    if( send(sockfd, my_message, strlen(my_message), 0) < 0)
                    {
                        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
                        return;
                    }

                    n = recv(sockfd, buff, MAXLINE, 0);
                    buff[n] = '\0';
                    printf("recv msg from server: %s\n", buff);

                    close(sockfd);
                }

                private:
                    std::unordered_map<ps::Key, sgdentry_w> store;
                    int push_num = 0;
                    int workers_num = 2;
                    char* center_ip = "112.31.12.177";
                    int center_port = 48080;

            };

            typedef struct SGDEntry_v
            {
                SGDEntry_v(int k = v_dim)
                {
                    w.resize(k, 0.001);
                }
                std::vector<float> w;
            } sgdentry_v;

            struct KVServerSGDHandle_v
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
                        v_dim = vals_size / keys_size;
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
                        SGDEntry_v& val = store[key];
                        for (int j = 0; j < v_dim; ++j)
                        {
                            if (req_meta.push)
                            {
                                float g = req_data.vals[i * v_dim + j];
                                val.w[j] -= learning_rate * g;
                            }
                            else
                            {
                                for (int j = 0; j < v_dim; ++j)
                                {
                                    res.vals[i * v_dim + j] = val.w[j];
                                }
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
                    std::unordered_map<ps::Key, sgdentry_v> store;
            };

        private:
    };
}  // namespace xflow

#endif  // SRC_OPTIMIZER_SGD_H_
