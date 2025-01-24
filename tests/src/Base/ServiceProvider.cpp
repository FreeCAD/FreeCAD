#include <gtest/gtest.h>
#include <Base/ServiceProvider.h>

class SimpleService
{
public:
    virtual ~SimpleService() = default;
    virtual std::string foo() = 0;

    SimpleService() = default;

    SimpleService(const SimpleService& other) = delete;
    SimpleService(SimpleService&& other) noexcept = delete;
    SimpleService& operator=(const SimpleService& other) = delete;
    SimpleService& operator=(SimpleService&& other) noexcept = delete;
};

class FirstServiceImplementation final: public SimpleService
{
public:
    std::string foo() override
    {
        return "first";
    }
};

class SecondServiceImplementation final: public SimpleService
{
public:
    std::string foo() override
    {
        return "second";
    }
};

TEST(ServiceProvider, provideEmptyImplementation)
{
    // Arrange
    Base::ServiceProvider serviceProvider;

    // Act
    auto implementation = serviceProvider.provide<SimpleService>();

    // Assert
    EXPECT_EQ(implementation, nullptr);
}

TEST(ServiceProvider, provideEmptyImplementationList)
{
    // Arrange
    Base::ServiceProvider serviceProvider;

    // Act
    const auto implementations = serviceProvider.all<SimpleService>();

    // Assert
    EXPECT_EQ(implementations.size(), 0);
}

TEST(ServiceProvider, provideImplementation)
{
    // Arrange
    Base::ServiceProvider serviceProvider;

    serviceProvider.registerImplementation<SimpleService>(new FirstServiceImplementation);

    // Act
    auto implementation = serviceProvider.provide<SimpleService>();

    // Assert
    EXPECT_NE(implementation, nullptr);
    EXPECT_EQ(implementation->foo(), "first");
}

TEST(ServiceProvider, provideLatestImplementation)
{
    // Arrange
    Base::ServiceProvider serviceProvider;

    serviceProvider.registerImplementation<SimpleService>(new FirstServiceImplementation);
    serviceProvider.registerImplementation<SimpleService>(new SecondServiceImplementation);

    // Act
    auto implementation = serviceProvider.provide<SimpleService>();

    // Assert
    EXPECT_NE(implementation, nullptr);
    EXPECT_EQ(implementation->foo(), "second");
}

TEST(ServiceProvider, provideAllImplementations)
{
    // Arrange
    Base::ServiceProvider serviceProvider;

    serviceProvider.registerImplementation<SimpleService>(new FirstServiceImplementation);
    serviceProvider.registerImplementation<SimpleService>(new SecondServiceImplementation);

    // Act
    auto implementations = serviceProvider.all<SimpleService>();
    auto it = implementations.begin();

    // Assert
    // Implementations should be available in order from the most recent one
    EXPECT_EQ((*it)->foo(), "second");
    ++it;
    EXPECT_EQ((*it)->foo(), "first");
    ++it;
    EXPECT_EQ(it, implementations.end());
}
