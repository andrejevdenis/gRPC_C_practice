#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <crtdbg.h>
#include <iostream>
#include <memory>

#include "fake_user_server.h"  // Подключаем наш ООП-сервер
#include "user_grpc_client.h"  // Подключаем наш ООП-клиент
#include "service.pb.h"       
#include "service.grpc.pb.h"  

// --- ТРЮК С ДИФФЕРЕНЦИАЦИЕЙ ПАМЯТИ ---
// Этот код выполнится автоматически при старте приложения до инициализации gRPC
struct MemoryLeakInitializer {
    MemoryLeakInitializer() {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
};
static MemoryLeakInitializer g_memory_leak_initializer;
// -------------------------------------

// Описываем сам тест с помощью Google Test
TEST(GrpcUserServiceTest, ReturnsCorrectGreetingForTom) {
    // 1. Фиксируем состояние кучи ДО старта бизнес-логики
    _CrtMemState s1, s2, s3;
    _CrtMemCheckpoint(&s1);

    { // <-- Изолирующий блок. Все ООП объекты внутри него гарантированно уничтожатся на строке }

        // Шаг 1: Поднимаем сервер и подключаем клиента в одну строку кода
        FakeUserServer server;
        UserGrpcClient client(server.GetInProcessChannel());

        // Шаг 2: ИСКУССТВЕННАЯ УТЕЧКА (наш тестовый массив на 400 байт)
        int* leaked_array = new int[100];
        leaked_array[0] = 42;

        // Шаг 3: Вызываем gRPC метод через чистый ООП интерфейс клиента
        std::string greeting;
        grpc::Status status = client.GetUserProfile("Tom", greeting);

        // Шаг 4: Проверяем утверждения (Валидация контракта)
        ASSERT_TRUE(status.ok());
        EXPECT_EQ(greeting, "Hi Tom from C++ Test!");

    } // <-- КОНЕЦ ОБЛАСТИ ВИДИМОСТИ: Здесь автоматически вызываются деструкторы ~UserGrpcClient() и ~FakeUserServer()

    // 2. Фиксируем состояние кучи ПОСЛЕ уничтожения gRPC компонентов
    _CrtMemCheckpoint(&s2);

    // 3. Вычисляем точную разницу на весах памяти
    if (_CrtMemDifference(&s3, &s1, &s2)) {
        std::cout << "\n==========================================" << std::endl;
        std::cout << "[MEMORY AUDIT] TOTAL LEAKED: " << s3.lSizes[_NORMAL_BLOCK] << " BYTES!" << std::endl;
        std::cout << "==========================================\n" << std::endl;

        // Печатаем подробный дамп только тех объектов, которые утекли именно здесь
        _CrtMemDumpAllObjectsSince(&s1);
    }
}
