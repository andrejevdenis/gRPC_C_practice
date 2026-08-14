#pragma once
#include <grpcpp/grpcpp.h>
#include <memory>
#include "service.grpc.pb.h"

class UserGrpcClient {
public:
    explicit UserGrpcClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(UserService::NewStub(channel)) {
    }

    // Метод для получения stub_ в потоки
    std::shared_ptr<UserService::Stub> GetStub() const {
        return stub_;
    }

    // Обычный синхронный метод
    grpc::Status GetUserProfile(const std::string& name, std::string& out_greeting) {
        UserRequest request;
        request.set_name(name);
        UserResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->GetUserProfile(&context, request, &response);
        if (status.ok()) {
            out_greeting = response.greeting();
        }
        return status;
    }

private:
    std::shared_ptr<UserService::Stub> stub_; // переводим на shared_ptr
};
