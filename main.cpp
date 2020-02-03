#include <iostream>
#include "modules.h"
#include "commands.h"

constexpr size_t word_size = 4;
constexpr size_t words = 10;

class top_module:public sc_module{
	memory mem;
	reader_writer rw;
public:
	SC_HAS_PROCESS(top_module);
	sc_in<bool> clk_in;

	top_module(sc_module_name name, sc_clock &clk)
		:sc_module(name),
		mem("mem", 1, word_size, words),
		rw("rw", word_size, words),
		clk_in("top_clk_in")
	{
		mem.mem_ports.at(0)->bind(rw.port);
		rw.clk_in(clk);
		this->clk_in(clk);

		SC_THREAD(func);
		sensitive<<clk_in.pos();
	}
	void func(){
		while(true){
			wait();
			std::cout<<sc_time_stamp()<<" in top\n";
			if(rw.get_ops() >= 16){
				sc_stop();
			}
		}
	}
};


int sc_main(int argc, char* argv[]){
	sc_clock clk("clk1",sc_time(2, SC_PS));
	top_module top("top", clk);
	sc_start();
	return 0;
}
