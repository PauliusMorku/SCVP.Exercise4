#include <systemc.h>
#include <iostream>

// Place Interface:
class placeInterface : public sc_interface
{
public:
    virtual void addTokens(void) = 0;
    virtual void removeTokens(void) = 0;
    virtual bool testTokens(void) = 0;
};

// Place Channel:
template<unsigned int I=1, unsigned int O=1>
class place : public placeInterface
{
public:
    void addTokens(void){tokens = tokens + I;}
    // TODO: not sure how the following function should behave when we remove more tokens than it has
    void removeTokens(void){
        if (tokens >= O)
            tokens = tokens - O;
        else
            tokens = 0;
    }
    bool testTokens(void){return tokens > 0;}

    place(unsigned int n) {tokens = n;}

private:
    unsigned int tokens;
};

// Transition:
template<unsigned int N=1, unsigned int M=1, unsigned int L=0>
SC_MODULE(transition)
{
public:
    sc_port<placeInterface, N, SC_ALL_BOUND> in;
    sc_port<placeInterface, M, SC_ALL_BOUND> out;
    sc_port<placeInterface, L, SC_ZERO_OR_MORE_BOUND> inhibitors; // SC_ZERO_OR_MORE_BOUND means that we can leave this port unbinded

    void fire(void)
    {
        // this loop checks if every input port contains atleast one token, if not - return is called
        for (unsigned int i = 0; i < N; i++)
        {
            if (in[i]->testTokens() == false)
            {
                std::cout << this->name() << ": NOT Fired" << std::endl;
                return;
            }
        }

        // we do not fire if there are tokens on inhibitor-arc
        for (unsigned int i = 0; i < L; i++)
        {
            if (inhibitors[i]->testTokens() == true)
            {
                std::cout << this->name() << ": NOT Fired" << std::endl;
                return;
            }
        }

        for (unsigned int i = 0; i < N; i++)
        {
            in[i]->removeTokens();
        }

        for (unsigned int i = 0; i < M; i++)
        {
            out[i]->addTokens();
        }

        std::cout << this->name() << ": Fired" << std::endl;
    }

    SC_CTOR(transition)
    {

    }
};

// TOPLEVEL
SC_MODULE(toplevel)
{
    public:
    transition<1,2> t1; // <?,?> passes parameters to module, <> leaves default parameters
    transition<2,1> t2;
    transition<1,1> t3;
    place<1,1> p1, p2, p3, p4;

    SC_CTOR(toplevel) : t1("t1"), t2("t2"), t3("t3"), p1(1), p2(0), p3(0), p4(0)
    {
        SC_THREAD(process);

        t1.in.bind(p1);
        t1.out.bind(p2);    // 0
        t1.out.bind(p3);    // 1
        t2.in.bind(p2);     // 0
        t2.in.bind(p4);     // 1
        t2.out.bind(p1);
        t3.in.bind(p3);
        t3.out.bind(p4);
    }

    void process()
    {
        while(true)
        {
            wait(10,SC_NS);
            t1.fire();
            wait(10,SC_NS);
            t2.fire();
            wait(10,SC_NS);
            t3.fire();
            wait(10,SC_NS);
            t2.fire();
            sc_stop();
        }
    }
};

// Memory Bank
SC_MODULE(memoryBank)
{
    public:
    transition<1,1> ACT;
    transition<1,1> RD;
    transition<1,1> PRE;
    transition<1,1> WR;
    place<1,1> IDLE;
    place<3,3> ACTIVE;

    SC_CTOR(memoryBank) : ACT("ACT"), RD("RD"), PRE("PRE"), WR("WR"), IDLE(1), ACTIVE(0)
    {
        SC_THREAD(process);

        ACT.in.bind(IDLE);
        ACT.out.bind(ACTIVE);
        RD.in.bind(ACTIVE);
        RD.out.bind(ACTIVE);
        PRE.in.bind(ACTIVE);
        PRE.out.bind(IDLE);
        WR.in.bind(ACTIVE);
        WR.out.bind(ACTIVE);
    }

    void process()
    {
        while(true)
        {
            wait(10,SC_NS);
            ACT.fire();
            wait(10,SC_NS);
            ACT.fire();
            wait(10,SC_NS);
            RD.fire();
            wait(10,SC_NS);
            WR.fire();
            wait(10,SC_NS);
            PRE.fire();
            wait(10,SC_NS);
            ACT.fire();
            sc_stop();
        }
    }
};

// Memory Bank used for Hierarchical PNs, it does not contain IDLE state and has inhibitor-arcs
SC_MODULE(memoryBankModified)
{
    public:
    transition<1,1,1> ACT; // include one inhibitor-arc
    transition<1,1> RD;
    transition<1,1> PRE;
    transition<1,1> WR;
    place<3,3> ACTIVE;

    SC_CTOR(memoryBankModified) : ACT("ACT"), RD("RD"), PRE("PRE"), WR("WR"), ACTIVE(0)
    {
        SC_THREAD(process);

        ACT.inhibitors.bind(ACTIVE); // connecting inhibitor-arc
        ACT.out.bind(ACTIVE);
        RD.in.bind(ACTIVE);
        RD.out.bind(ACTIVE);
        PRE.in.bind(ACTIVE);
        WR.in.bind(ACTIVE);
        WR.out.bind(ACTIVE);
    }

    void process()
    {
        while(true)
        {
            wait(10,SC_NS);
            ACT.fire();
            wait(10,SC_NS);
            ACT.fire();
            wait(10,SC_NS);
            RD.fire();
            wait(10,SC_NS);
            WR.fire();
            wait(10,SC_NS);
            PRE.fire();
            wait(10,SC_NS);
            ACT.fire();
            sc_stop();
        }
    }
};

// Two Memory Banks
SC_MODULE(twoMemoryBanks)
{
    public:
    memoryBankModified s1;
    memoryBankModified s2;
    place<1,1> IDLE;

    SC_CTOR(twoMemoryBanks) : s1("s1"), s2("s2"), IDLE(2)
    {
        SC_THREAD(process);

        s1.ACT.in.bind(IDLE);
        s2.ACT.in.bind(IDLE);
        s1.PRE.out.bind(IDLE);
        s2.PRE.out.bind(IDLE);
    }

    void process()
    {
        while(true)
        {
            wait(10,SC_NS);
            s1.ACT.fire();
            wait(10,SC_NS);
            s1.ACT.fire();
            wait(10,SC_NS);
            s1.RD.fire();
            wait(10,SC_NS);
            s1.WR.fire();
            wait(10,SC_NS);
            s1.PRE.fire();
            wait(10,SC_NS);
            s1.ACT.fire();
            wait(10,SC_NS);
            s2.ACT.fire();
            wait(10,SC_NS);
            s2.ACT.fire();
            wait(10,SC_NS);
            s1.PRE.fire();
            wait(10,SC_NS);
            s2.PRE.fire();
            wait(10,SC_NS);
            sc_stop();
        }
    }
};

int sc_main(int argc, char* argv[])
{
    // uncomment module to simulate it
    //toplevel t("top");
    //memoryBank mb("memoryBank");
    twoMemoryBanks tmb("twoMemoryBanks");

    /* TODO: ask why do I get following warning when running twoMemoryBanks.
     * Warning: (W545) sc_stop has already been called
     * In file: sc_simcontext.cpp:1047
     * In process: twoMemoryBanks.s2.process @ 60 ns
     *
     * Other questions:
     * So binding means that we can call binded object functions, because otherwise we would only have interface without implementation?
     */

    sc_start();
    return 0;
}
