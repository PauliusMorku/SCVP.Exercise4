#include <systemc.h>
#include <iostream>

// Place Interface:
//template <class T>
class placeInterface : public sc_interface
{
public:
    virtual void addTokens(unsigned int n) = 0;
    virtual void removeTokens(unsigned int n) = 0;
    virtual unsigned int testTokens(void) = 0;
};

// Place Channel:
class place : public placeInterface
{
public:
    void addTokens(unsigned int n){tokens = tokens + n;}
    void removeTokens(unsigned int n){tokens = tokens - n;}
    unsigned int testTokens(void){return tokens;}

    place(unsigned int n) {tokens = n;}

private:
    unsigned int tokens;
};

// Transition:
template<unsigned int N=1, unsigned int M=1>
SC_MODULE(transition)
{
public:
    sc_port<placeInterface> in;
    sc_port<placeInterface> out;

    void fire(void)
    {

        if (in->testTokens() > 0)
        {
            std::cout << this->name() << ": Fired" << std::endl;
            in->removeTokens(1);
        }
        else
        {
            std::cout << this->name() << ": NOT Fired" << std::endl;
            out->addTokens(1);
        }
    }

    SC_CTOR(transition)
    {

    }

};

// TOPLEVEL
SC_MODULE(toplevel)
{
    public:
    transition<1,1> t1, t2;
    place p1, p2;

    SC_CTOR(toplevel) : t1("t1"), t2("t2"), p1(1), p2(0)
    {
        SC_THREAD(process);
        t1.in.bind(p1);
        t1.out.bind(p2);
        t2.in.bind(p2);
        t2.out.bind(p1);
    }

    void process()
    {
        while(true)
        {
            wait(10,SC_NS);
            t1.fire();
            wait(10,SC_NS);
            t1.fire();
            wait(10,SC_NS);
            t2.fire();
            sc_stop();
        }
    }
};

int sc_main(int argc, char* argv[])
{
    toplevel t("top");



    sc_start();
    return 0;
}
