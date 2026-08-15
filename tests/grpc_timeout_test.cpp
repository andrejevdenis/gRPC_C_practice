#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iostream>
#include <memory>

#include "fake_user_server.h"  // Используем наш общий ООП-сервер!
#include "user_grpc_client.h"  // Используем наш общий ООП-клиент!
#include "service.pb.h"       
#include "service.grpc.pb.h"  

TEST(GrpcTimeoutTest, DropsConnectionOnServerHang) {
    // 1. Инициализируем общие ООП-компоненты
    FakeUserServer server;
    UserGrpcClient client(server.GetInProcessChannel());

    // 2. ДИНАМИЧЕСКИ ВКЛЮЧАЕМ ЗАДЕРЖКУ: Просим сервер зависнуть на 3 секунды
    server.SetServerDelay(3);

    // Достаем потокобезопасный stub для ручной настройки дедлайна
    std::shared_ptr<UserService::Stub> client_stub = client.GetStub();

    UserRequest request;
    request.set_name("Tom");
    UserResponse response;

    // 3. НАСТРОЙКА ТАЙМАУТА КЛИЕНТА
    grpc::ClientContext context;
    // Клиент готов ждать ровно 1 секунду
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
    context.set_deadline(deadline);

    // 4. ВЫЗОВ СЕРВЕРА
    grpc::Status status = client_stub->GetUserProfile(&context, request, &response);

    // 5. ВАЛИДАЦИЯ (Тест успешен, если gRPC оборвал связь по DEADLINE_EXCEEDED)
    ASSERT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::DEADLINE_EXCEEDED);

    std::cout << "\n==========================================" << std::endl;
    std::cout << "[TIMEOUT TEST] Connection aborted successfully!" << std::endl;
    std::cout << "[TIMEOUT TEST] Status Code: " << status.error_code() << std::endl;
    std::cout << "[TIMEOUT TEST] Error Message: " << status.error_message() << std::endl;
    std::cout << "==========================================\n" << std::endl;
}