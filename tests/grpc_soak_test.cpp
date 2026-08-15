#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <algorithm> 

#include "fake_user_server.h"
#include "user_grpc_client.h"
#include "service.pb.h"       
#include "service.grpc.pb.h"  

TEST(GrpcSoakTest, MeasureRandomTimeoutDuration) {
    // Вся магия 1-миллисекундного таймера теперь происходит внутри этой строчки!
    FakeUserServer server;
    UserGrpcClient client(server.GetInProcessChannel());

    std::shared_ptr<UserService::Stub> client_stub = client.GetStub();
    UserRequest request;
    request.set_name("Tom");
    UserResponse response;

    const int RUNS = 50;
    const int CLIENT_TIMEOUT_MS = 500;

    std::random_device rd;
    std::mt19937 gen(rd());

    int min_delay_ms = 300;
    int max_delay_ms = 700;
    std::uniform_int_distribution<> distrib(min_delay_ms, max_delay_ms);

    std::cout << "\n================ START HIGH-PRECISION CHAOS TEST ================" << std::endl;
    std::cout << "Running " << RUNS << " iterations. Client timeout: " << CLIENT_TIMEOUT_MS << " ms" << std::endl;

    int timeout_count = 0;
    int success_count = 0;

    long long max_success_ms = 0;
    long long min_success_ms = LLONG_MAX;
    long long max_timeout_ms = 0;
    long long min_timeout_ms = LLONG_MAX;

    for (int i = 1; i <= RUNS; ++i) {
        int random_server_delay_ms = distrib(gen);
        server.SetServerDelayMs(random_server_delay_ms);

        grpc::ClientContext context;
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(CLIENT_TIMEOUT_MS);
        context.set_deadline(deadline);

        auto start = std::chrono::high_resolution_clock::now();
        grpc::Status status = client_stub->GetUserProfile(&context, request, &response);
        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "[RUN #" << i << "] Server Delay: " << random_server_delay_ms << " ms | ";

        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
            std::cout << "🔴 ABORTED (Timeout) after " << elapsed_ms << " ms" << std::endl;
            timeout_count++;
            max_timeout_ms = std::max(max_timeout_ms, elapsed_ms);
            min_timeout_ms = std::min(min_timeout_ms, elapsed_ms);
        }
        else if (status.ok()) {
            std::cout << "🟢 SUCCESS (Responded) after " << elapsed_ms << " ms" << std::endl;
            success_count++;
            max_success_ms = std::max(max_success_ms, elapsed_ms);
            min_success_ms = std::min(min_success_ms, elapsed_ms);
        }
    }

    std::cout << "\n=================== FINAL STATISTICS ===================" << std::endl;
    std::cout << "Total Runs: " << RUNS << std::endl;
    std::cout << "Successful responses (Server won): " << success_count << std::endl;
    if (success_count > 0) {
        std::cout << "  -> Min success time: " << min_success_ms << " ms" << std::endl;
        std::cout << "  -> Max success time: " << max_success_ms << " ms" << std::endl;
    }
    std::cout << "Connections aborted (Client won):  " << timeout_count << std::endl;
    if (timeout_count > 0) {
        std::cout << "  -> Min timeout time: " << min_timeout_ms << " ms" << std::endl;
        std::cout << "  -> Max timeout time: " << max_timeout_ms << " ms" << std::endl;
    }
    std::cout << "========================================================\n" << std::endl;
}
