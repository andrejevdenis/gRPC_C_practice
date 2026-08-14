#pragma once
#include <grpcpp/grpcpp.h>
#include "service.grpc.pb.h"

// Чистая реализация gRPC сервиса
class FakeUserServiceImpl final : public UserService::Service {
    grpc::Status GetUserProfile(grpc::ServerContext* context, const UserRequest* request, UserResponse* response) override {
        if (request->name() == "Tom") {
            response->set_greeting("Hi Tom from C++ Test!");
            return grpc::Status::OK;
        }
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
    }
};

// Класс-менеджер сервера (ООП оболочка)
class FakeUserServer {
public:
    FakeUserServer() {
        builder_.AddListeningPort("localhost:0", grpc::InsecureServerCredentials());
        builder_.RegisterService(&service_impl_);
        server_ = builder_.BuildAndStart();
    }

    ~FakeUserServer() {
        if (server_) {
            server_->Shutdown();
        }
    }

    // Позволяет клиентам подключаться к внутреннему каналу этого сервера
    std::shared_ptr<grpc::Channel> GetInProcessChannel() {
        return server_->InProcessChannel(grpc::ChannelArguments());
    }

private:
    FakeUserServiceImpl service_impl_;
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_;
};