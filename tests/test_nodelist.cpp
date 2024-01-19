#include "assert_exception.h"
#include <figcone/config.h>
#include <figcone/configreader.h>
#include <figcone/errors.h>
#include <figcone_tree/tree.h>
#include <gtest/gtest.h>

#include <deque>
#include <list>

#if __has_include(<figcone/detail/external/nameof.hpp>)
#define NAMEOF_AVAILABLE
#endif

namespace test_nodelist {

struct NonAggregateNode : public figcone::Config {
    using Config::Config;
    virtual ~NonAggregateNode() = default;
    FIGCONE_PARAM(testInt, int);
};

struct Node : public figcone::Config {
    FIGCONE_PARAM(testInt, int);
};

struct NonAggregateCfg : public figcone::Config {
    FIGCONE_NODELIST(testNodes, std::vector<NonAggregateNode>);
    FIGCONE_NODELIST(optTestNodes, std::list<NonAggregateNode>)();
    FIGCONE_NODELIST(optTestNodes2, figcone::optional<std::deque<NonAggregateNode>>);
    FIGCONE_PARAM(testStr, std::string);
};

struct Cfg : public figcone::Config {
    FIGCONE_NODELIST(testNodes, std::vector<Node>);
    FIGCONE_NODELIST(optTestNodes, std::list<Node>)();
    FIGCONE_NODELIST(optTestNodes2, figcone::optional<std::deque<Node>>);
    FIGCONE_PARAM(testStr, std::string);
};

struct Cfg2 : public figcone::Config {
    FIGCONE_PARAM(testDouble, double);
    FIGCONE_NODE(testNode, Node);
};

struct NotLessThan {
public:
    NotLessThan(int minSize)
        : minSize_{minSize}
    {
    }

    void operator()(const std::vector<Node>& nodeList)
    {
        if (static_cast<int>(nodeList.size()) < minSize_)
            throw figcone::ValidationError{"can't have less than 2 elements"};
    }

private:
    int minSize_;
};

struct ValidatedCfg : public figcone::Config {
    FIGCONE_NODELIST(testNodes, std::vector<Node>)
            .ensure(
                    [](const std::vector<Node>& nodeList)
                    {
                        if (nodeList.size() < 2)
                            throw figcone::ValidationError{"can't have less than 2 elements"};
                    });
};

struct ValidatedOptionalNodeListCfg : public figcone::Config {
    FIGCONE_NODELIST(testNodesOpt, figcone::optional<std::vector<Node>>)
            .ensure(
                    [](const std::vector<Node>& nodeList)
                    {
                        if (nodeList.size() < 2)
                            throw figcone::ValidationError{"can't have less than 2 elements"};
                    });
};

struct ValidatedWithFunctorCfg : public figcone::Config {
    FIGCONE_NODELIST(testNodes, std::vector<Node>).ensure<NotLessThan>(2);
};

struct ValidatedWithFunctorOptionalNodeListCfg : public figcone::Config {
    FIGCONE_NODELIST(testNodesOpt, figcone::optional<std::vector<Node>>).ensure<NotLessThan>(2);
};

#ifdef NAMEOF_AVAILABLE
struct CfgWithoutMacro : public figcone::Config {
    std::vector<Node> testNodes = nodeList<&CfgWithoutMacro::testNodes>();
};
#else
struct CfgWithoutMacro : public figcone::Config {
    std::vector<Node> testNodes = nodeList<&CfgWithoutMacro::testNodes>("testNodes");
};
#endif

struct NestedCfgList : public figcone::Config {
    FIGCONE_PARAM(testStr, std::string);
    FIGCONE_NODELIST(testList, std::deque<Cfg>);
};

class TreeProvider : public figcone::IParser {
public:
    TreeProvider(std::unique_ptr<figcone::TreeNode> tree)
        : tree_{std::move(tree)}
    {
    }

    figcone::Tree parse(std::istream&) override
    {
        return std::move(tree_);
    }

    std::unique_ptr<figcone::TreeNode> tree_;
};

TEST(TestNodeList, Basic)
{
    ///testStr = Hello
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 2
    ///[[optTestNodes]]
    ///   testInt = 100
    ///[[optTestNodes2]]
    ///   testInt = 200
    ///[[optTestNodes2]]
    ///   testInt = 300
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testNodes = tree->asItem().addNodeList("testNodes", {2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {3, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({4, 1});
        node.asItem().addParam("testInt", "2", {5, 3});
    }
    auto& optTestNodes = tree->asItem().addNodeList("optTestNodes", {6, 1});
    {
        auto& node = optTestNodes.asList().emplaceBack({6, 1});
        node.asItem().addParam("testInt", "100", {7, 3});
    }
    auto& optTestNodes2 = tree->asItem().addNodeList("optTestNodes2", {8, 1});
    {
        {
            auto& node = optTestNodes2.asList().emplaceBack({8, 1});
            node.asItem().addParam("testInt", "200", {9, 3});
        }
        {
            auto& node = optTestNodes2.asList().emplaceBack({10, 1});
            node.asItem().addParam("testInt", "300", {11, 3});
        }
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<Cfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 2);
    ASSERT_EQ(cfg.optTestNodes.size(), 1);
    EXPECT_EQ(cfg.optTestNodes.front().testInt, 100);
    ASSERT_TRUE(cfg.optTestNodes2);
    ASSERT_EQ(cfg.optTestNodes2->size(), 2);
    ASSERT_EQ(cfg.optTestNodes2->at(0).testInt, 200);
    ASSERT_EQ(cfg.optTestNodes2->at(1).testInt, 300);
    EXPECT_EQ(cfg.testStr, "Hello");
}

TEST(TestNodeList, BasicAnyNode)
{
    ///testStr = Hello
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 2
    ///[[optTestNodes]]
    ///   testInt = 100
    ///[[optTestNodes2]]
    ///   testInt = 200
    ///[[optTestNodes2]]
    ///   testInt = 300
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testNodes = tree->asItem().addAny("testNodes", {2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {3, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({4, 1});
        node.asItem().addParam("testInt", "2", {5, 3});
    }
    auto& optTestNodes = tree->asItem().addNodeList("optTestNodes", {6, 1});
    {
        auto& node = optTestNodes.asList().emplaceBackAny({6, 1});
        node.asItem().addParam("testInt", "100", {7, 3});
    }
    auto& optTestNodes2 = tree->asItem().addAny("optTestNodes2", {8, 1});
    {
        {
            auto& node = optTestNodes2.asList().emplaceBackAny({8, 1});
            node.asItem().addParam("testInt", "200", {9, 3});
        }
        {
            auto& node = optTestNodes2.asList().emplaceBackAny({10, 1});
            node.asItem().addParam("testInt", "300", {11, 3});
        }
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<Cfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 2);
    ASSERT_EQ(cfg.optTestNodes.size(), 1);
    EXPECT_EQ(cfg.optTestNodes.front().testInt, 100);
    ASSERT_TRUE(cfg.optTestNodes2);
    ASSERT_EQ(cfg.optTestNodes2->size(), 2);
    ASSERT_EQ(cfg.optTestNodes2->at(0).testInt, 200);
    ASSERT_EQ(cfg.optTestNodes2->at(1).testInt, 300);
    EXPECT_EQ(cfg.testStr, "Hello");
}

TEST(TestNodeList, BasicNonAggregate)
{
    ///testStr = Hello
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 2
    ///[[optTestNodes]]
    ///   testInt = 100
    ///[[optTestNodes2]]
    ///   testInt = 200
    ///[[optTestNodes2]]
    ///   testInt = 300
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testNodes = tree->asItem().addNodeList("testNodes", {2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {3, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({4, 1});
        node.asItem().addParam("testInt", "2", {5, 3});
    }
    auto& optTestNodes = tree->asItem().addNodeList("optTestNodes", {6, 1});
    {
        auto& node = optTestNodes.asList().emplaceBack({6, 1});
        node.asItem().addParam("testInt", "100", {7, 3});
    }
    auto& optTestNodes2 = tree->asItem().addNodeList("optTestNodes2", {8, 1});
    {
        {
            auto& node = optTestNodes2.asList().emplaceBack({8, 1});
            node.asItem().addParam("testInt", "200", {9, 3});
        }
        {
            auto& node = optTestNodes2.asList().emplaceBack({10, 1});
            node.asItem().addParam("testInt", "300", {11, 3});
        }
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<NonAggregateCfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 2);
    ASSERT_EQ(cfg.optTestNodes.size(), 1);
    EXPECT_EQ(cfg.optTestNodes.front().testInt, 100);
    ASSERT_TRUE(cfg.optTestNodes2);
    ASSERT_EQ(cfg.optTestNodes2->size(), 2);
    ASSERT_EQ(cfg.optTestNodes2->at(0).testInt, 200);
    ASSERT_EQ(cfg.optTestNodes2->at(1).testInt, 300);
    EXPECT_EQ(cfg.testStr, "Hello");
}

TEST(TestNodeList, BasicAnyWithoutOptional)
{

    ///testStr = Hello
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 2
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testNodes = tree->asItem().addNodeList("testNodes", {2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {3, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({4, 1});
        node.asItem().addParam("testInt", "2", {5, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<Cfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 2);
    ASSERT_EQ(cfg.optTestNodes.size(), 0);
    ASSERT_FALSE(cfg.optTestNodes2);
    EXPECT_EQ(cfg.testStr, "Hello");
}

TEST(TestNodeList, BasicWithoutMacro)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 2
    ///
    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({3, 1});
        node.asItem().addParam("testInt", "2", {4, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<CfgWithoutMacro>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 2);
}

TEST(TestNodeList, ValidationSuccess)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 4

    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({3, 1});
        node.asItem().addParam("testInt", "4", {4, 3});
    }
    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<ValidatedCfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 4);
}

TEST(TestNodeList, ValidationSuccessOptionalNodeList)
{
    ///[[testNodesOpt]]
    ///  testInt = 5
    ///[[testNodesOpt]]
    ///  testInt = 6
    auto tree = figcone::makeTreeRoot();
    auto& testNodesOpt = tree->asItem().addNodeList("testNodesOpt", {5, 1});
    {
        auto& node = testNodesOpt.asList().emplaceBack({5, 1});
        node.asItem().addParam("testInt", "5", {6, 3});
    }
    {
        auto& node = testNodesOpt.asList().emplaceBack({7, 1});
        node.asItem().addParam("testInt", "6", {8, 3});
    }
    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<ValidatedOptionalNodeListCfg>("", parser);
    ASSERT_TRUE(cfg.testNodesOpt.has_value());
    ASSERT_EQ(cfg.testNodesOpt->size(), 2);
    EXPECT_EQ(cfg.testNodesOpt->at(0).testInt, 5);
    EXPECT_EQ(cfg.testNodesOpt->at(1).testInt, 6);
}

TEST(TestNodeList, ValidationFailure)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///
    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<ValidatedCfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:1, column:1] Node list 'testNodes': can't have less than 2 elements");
            });
}

TEST(TestNodeList, ValidationFailureOptionalNodeList)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///
    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodesOpt", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<ValidatedOptionalNodeListCfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:1, column:1] Node list 'testNodesOpt': can't have less than 2 elements");
            });
}

TEST(TestNodeList, ValidationWithFunctorSuccess)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 4

    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({3, 1});
        node.asItem().addParam("testInt", "4", {4, 3});
    }
    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<ValidatedWithFunctorCfg>("", parser);

    ASSERT_EQ(cfg.testNodes.size(), 2);
    EXPECT_EQ(cfg.testNodes[0].testInt, 3);
    EXPECT_EQ(cfg.testNodes[1].testInt, 4);
}

TEST(TestNodeList, ValidationWithFunctorSuccessOptionalNodeList)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///[[testNodes]]
    ///  testInt = 4

    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodesOpt", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({3, 1});
        node.asItem().addParam("testInt", "4", {4, 3});
    }
    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<ValidatedWithFunctorOptionalNodeListCfg>("", parser);

    ASSERT_EQ(cfg.testNodesOpt.has_value(), true);
    ASSERT_EQ(cfg.testNodesOpt->size(), 2);
    EXPECT_EQ(cfg.testNodesOpt->at(0).testInt, 3);
    EXPECT_EQ(cfg.testNodesOpt->at(1).testInt, 4);
}

TEST(TestNodeList, ValidationWithFunctorFailure)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///
    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<ValidatedWithFunctorCfg>(
                        R"(
        #testNodes:
        ###
            testInt = 3
    )",
                        parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:1, column:1] Node list 'testNodes': can't have less than 2 elements");
            });
}

TEST(TestNodeList, ValidationWithFunctorFailureOptionalNodeList)
{
    ///[[testNodes]]
    ///  testInt = 3
    ///
    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodesOpt", {1, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "3", {2, 3});
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<ValidatedWithFunctorOptionalNodeListCfg>(
                        R"(
        #testNodes:
        ###
            testInt = 3
    )",
                        parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:1, column:1] Node list 'testNodesOpt': can't have less than 2 elements");
            });
}

TEST(TestNodeList, NestedCfgList)
{
    ///testStr = Hello
    ///[[testList]]
    ///  testStr = Hello
    ///  [[testList.testNodes]]
    ///    testInt = 3
    ///  [[testList.testNodes]]
    ///    testInt = 33
    ///[[testList]]
    ///  testStr = World
    ///  [[testList.testNodes]]
    ///    testInt = 5
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testList = tree->asItem().addNodeList("testList", {2, 1});
    {
        auto& node = testList.asList().emplaceBack({2, 1});
        node.asItem().addParam("testStr", "Hello", {3, 3});
        auto& testNodes = node.asItem().addNodeList("testNodes", {4, 3});
        {
            auto& newNode = testNodes.asList().emplaceBack({4, 3});
            newNode.asItem().addParam("testInt", "3", {5, 3});
        }
        {
            auto& newNode = testNodes.asList().emplaceBack({6, 3});
            newNode.asItem().addParam("testInt", "33", {7, 3});
        }
    }
    {
        auto& node = testList.asList().emplaceBack({8, 1});
        node.asItem().addParam("testStr", "World", {8, 3});
        auto& testNodes = node.asItem().addNodeList("testNodes", {9, 3});
        {
            auto& newNode = testNodes.asList().emplaceBack({9, 3});
            newNode.asItem().addParam("testInt", "5", {10, 5});
        }
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<NestedCfgList>("", parser);

    EXPECT_EQ(cfg.testStr, "Hello");
    ASSERT_EQ(cfg.testList.size(), 2);
    EXPECT_EQ(cfg.testList.at(0).testStr, "Hello");
    ASSERT_EQ(cfg.testList.at(0).testNodes.size(), 2);
    EXPECT_EQ(cfg.testList.at(0).testNodes.at(0).testInt, 3);
    EXPECT_EQ(cfg.testList.at(0).testNodes.at(1).testInt, 33);
    EXPECT_EQ(cfg.testList.at(1).testStr, "World");
    ASSERT_EQ(cfg.testList.at(1).testNodes.size(), 1);
    EXPECT_EQ(cfg.testList.at(1).testNodes.at(0).testInt, 5);
}

TEST(TestNodeList, NestedCfgAnyList)
{
    ///testStr = Hello
    ///[[testList]]
    ///  testStr = Hello
    ///  [[testList.testNodes]]
    ///    testInt = 3
    ///  [[testList.testNodes]]
    ///    testInt = 33
    ///[[testList]]
    ///  testStr = World
    ///  [[testList.testNodes]]
    ///    testInt = 5
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    auto& testList = tree->asItem().addAny("testList", {2, 1});
    {
        auto& node = testList.asList().emplaceBackAny({2, 1});
        node.asItem().addParam("testStr", "Hello", {3, 3});
        auto& testNodes = node.asItem().addAny("testNodes", {4, 3});
        {
            auto& newNode = testNodes.asList().emplaceBackAny({4, 3});
            newNode.asItem().addParam("testInt", "3", {5, 3});
        }
        {
            auto& newNode = testNodes.asList().emplaceBackAny({6, 3});
            newNode.asItem().addParam("testInt", "33", {7, 3});
        }
    }
    {
        auto& node = testList.asList().emplaceBackAny({8, 1});
        node.asItem().addParam("testStr", "World", {8, 3});
        auto& testNodes = node.asItem().addAny("testNodes", {9, 3});
        {
            auto& newNode = testNodes.asList().emplaceBackAny({9, 3});
            newNode.asItem().addParam("testInt", "5", {10, 5});
        }
    }

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    auto cfg = cfgReader.read<NestedCfgList>("", parser);

    EXPECT_EQ(cfg.testStr, "Hello");
    ASSERT_EQ(cfg.testList.size(), 2);
    EXPECT_EQ(cfg.testList.at(0).testStr, "Hello");
    ASSERT_EQ(cfg.testList.at(0).testNodes.size(), 2);
    EXPECT_EQ(cfg.testList.at(0).testNodes.at(0).testInt, 3);
    EXPECT_EQ(cfg.testList.at(0).testNodes.at(1).testInt, 33);
    EXPECT_EQ(cfg.testList.at(1).testStr, "World");
    ASSERT_EQ(cfg.testList.at(1).testNodes.size(), 1);
    EXPECT_EQ(cfg.testList.at(1).testNodes.at(0).testInt, 5);
}

TEST(TestNodeList, MissingNodeListError)
{
    ///testStr = Hello
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                auto cfg = cfgReader.read<Cfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(std::string{error.what()}, "[line:1, column:1] Root node: Node 'testNodes' is missing.");
            });
}

TEST(TestNodeList, NodeNotListError)
{
    ///testStr = Hello
    ///
    auto tree = figcone::makeTreeRoot();
    tree->asItem().addParam("testStr", "Hello", {1, 1});
    tree->asItem().addNode("testNodes", {2, 1});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<Cfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:2, column:1] Node list 'testNodes': config node must be a list.");
            });
}

TEST(TestNodeList, InvalidListElementError)
{
    ///testStr = Hello
    ///[[testNodes]]
    ///    testInt = error
    ///[[testNodes]]
    ///    testInt = 2

    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({2, 1});
        node.asItem().addParam("testInt", "error", {3, 3});
    }
    {
        auto& node = testNodes.asList().emplaceBack({4, 1});
        node.asItem().addParam("testInt", "2", {5, 3});
    }
    tree->asItem().addParam("testStr", "Hello", {1, 1});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<Cfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:3, column:3] Couldn't set parameter 'testInt' value from 'error'");
            });
}

TEST(TestNodeList, IncompleteListElementError)
{
    ///testStr = Hello
    ///[[testNodes]]
    ///[[testNodes]]
    ///    testInt = 2

    auto tree = figcone::makeTreeRoot();
    auto& testNodes = tree->asItem().addNodeList("testNodes", {2, 1});
    testNodes.asList().emplaceBack({2, 1});
    {
        auto& node = testNodes.asList().emplaceBack({3, 1});
        node.asItem().addParam("testInt", "2", {4, 3});
    }
    tree->asItem().addParam("testStr", "Hello", {1, 1});

    auto parser = TreeProvider{std::move(tree)};
    auto cfgReader = figcone::ConfigReader{figcone::NameFormat::CamelCase};
    assert_exception<figcone::ConfigError>(
            [&]
            {
                cfgReader.read<Cfg>("", parser);
            },
            [](const figcone::ConfigError& error)
            {
                EXPECT_EQ(
                        std::string{error.what()},
                        "[line:2, column:1] Node list 'testNodes': Parameter 'testInt' is missing.");
            });
}

} //namespace test_nodelist
