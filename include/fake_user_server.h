#pragma once
#include <grpcpp/grpcpp.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "service.grpc.pb.h"

// Подключаем системные заголовки Windows только для Win-платформы
#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

// Чистая реализация gRPC сервиса
class FakeUserServiceImpl final : public UserService::Service {
public:
    FakeUserServiceImpl() : delay_ms_(0) {}

    void SetDelayMs(int milliseconds) {
        delay_ms_ = milliseconds;
    }

    grpc::Status GetUserProfile(grpc::ServerContext* context, const UserRequest* request, UserResponse* response) override {
        if (delay_ms_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
        }

        if (request->name() == "Tom") {
            response->set_greeting("Hi Tom from C++ Test!");
            return grpc::Status::OK;
        }
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
    }

private:
    std::atomic<int> delay_ms_;
};

// Класс-менеджер сервера (ООП оболочка)
class FakeUserServer {
public:
    FakeUserServer() {
        // --- ПОВЫШАЕМ ТОЧНОСТЬ ТАЙМЕРОВ WINDOWS ДО 1 МС ---
#ifdef _WIN32
        timeBeginPeriod(1);
#endif
        // --------------------------------------------------

        builder_.AddListeningPort("localhost:0", grpc::InsecureServerCredentials());
        builder_.RegisterService(&service_impl_);
        server_ = builder_.BuildAndStart();
    }

    ~FakeUserServer() {
        if (server_) {
            server_->Shutdown();
        }

        // --- ВОЗВРАЩАЕМ СИСТЕМНЫЙ ТАЙМЕР К СТАНДАРТНЫМ 15 МС ---
#ifdef _WIN32
        timeEndPeriod(1);
#endif
        // ------------------------------------------------------
    }

    void SetServerDelayMs(int milliseconds) {
        service_impl_.SetDelayMs(milliseconds);
    }

    // Backwards-compatible wrapper: tests expect SetServerDelay(...)
    void SetServerDelay(int milliseconds) {
        SetServerDelayMs(milliseconds);
    }

    std::shared_ptr<grpc::Channel> GetInProcessChannel() {
        return server_->InProcessChannel(grpc::ChannelArguments());
    }

private:
    FakeUserServiceImpl service_impl_;
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_;
};
