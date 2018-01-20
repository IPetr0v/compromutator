#pragma once

#include "../src/NetworkSpace.hpp"
#include "../src/network/Network.hpp"

#include <memory>

class ExampleNetwork
{
protected:
    virtual void initNetwork() = 0;
    virtual void destroyNetwork() = 0;
};

class SimpleTwoSwitchNetwork : public ExampleNetwork
{
protected:
    using H = HeaderSpace;
    using N = NetworkSpace;

    void initNetwork() override {
        network = std::make_shared<Network>();
        std::vector<PortId> ports{1,2};
        sw1 = network->addSwitch(1, ports, 2);
        sw2 = network->addSwitch(2, ports, 1);

        port11 = sw1->port(1);
        port12 = sw1->port(2);
        port21 = sw2->port(1);
        port22 = sw2->port(2);

        rule1 = network->addRule(1, 0, 1, N(1, H("0000xxxx")),
                                 ActionsBase::portAction(2));
        table_miss1 = network->getSwitch(1)->table(0)->tableMissRule();

        rule2 = network->addRule(2, 0, 1, N(1, H("000000xx")),
                                 ActionsBase::portAction(2));
        table_miss2 = network->getSwitch(2)->table(0)->tableMissRule();
    }

    void destroyNetwork() override {
        network.reset();
    }

    std::shared_ptr<Network> network;
    SwitchPtr sw1, sw2;
    PortPtr port11, port12, port21, port22;
    RulePtr rule1, rule2;
    RulePtr table_miss1, table_miss2;
};

class TwoSwitchNetwork : public ExampleNetwork
{
protected:
    using H = HeaderSpace;
    using N = NetworkSpace;

    void initNetwork() override {
        network = std::make_shared<Network>();
        std::vector<PortId> ports{1,2};
        sw1 = network->addSwitch(1, ports, 2);
        sw2 = network->addSwitch(2, ports, 1);

        port11 = sw1->port(1);
        port12 = sw1->port(2);
        port21 = sw2->port(1);
        port22 = sw2->port(2);

        rule1 = network->addRule(1, 0, 3, N(1, H("00000000")),
                                 ActionsBase::tableAction(1));
        rule2 = network->addRule(1, 0, 2, N(1, H("0000xxxx")),
                                 ActionsBase::portAction(2));

        link = network->addLink({1,2}, {2,1}).first;

        rule3 = network->addRule(1, 0, 2, N(1, H("0011xxxx")),
                                 ActionsBase::portAction(2));
        rule4 = network->addRule(1, 0, 2, N(H("11111111")),
                                 ActionsBase::dropAction());
        rule5 = network->addRule(1, 0, 1, N(H("1111xxxx")),
                                 ActionsBase::portAction(2));
        table_miss = network->getSwitch(1)->table(0)->tableMissRule();

        rule11 = network->addRule(1, 1, 1, N(1, H("00xxxxxx")),
                                  ActionsBase::portAction(2));
        table_miss1 = network->getSwitch(1)->table(1)->tableMissRule();

        rule1_sw2 = network->addRule(2, 0, 1, N(1, H("00000000")),
                                  ActionsBase::portAction(2));
        table_miss_sw2 = network->getSwitch(2)->table(0)->tableMissRule();
    }

    void destroyNetwork() override {
        network.reset();
    }

    std::shared_ptr<Network> network;
    SwitchPtr sw1, sw2;
    PortPtr port11, port12, port21, port22;
    Link link;
    RulePtr rule1, rule2, rule3, rule4, rule5, table_miss;
    RulePtr rule11, table_miss1;
    RulePtr rule1_sw2, table_miss_sw2;
};
