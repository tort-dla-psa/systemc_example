#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <random>
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xio.hpp>
#include "commands.h"

class memory:public sc_module{
	static xt::xarray<uint8_t> bytes;
public:
	using mem_port = tlm_utils::simple_target_socket<memory>;
	std::vector<std::unique_ptr<mem_port>> mem_ports;

	SC_HAS_PROCESS(memory);
	memory(sc_module_name name, size_t ports, size_t word_size, size_t words)
		:sc_module(name)
	{
		bytes = xt::zeros<uint8_t>({word_size, words});
		for(size_t i=0; i<ports; i++){
			auto name = "memport"+std::to_string(i);
			mem_ports.emplace_back(new mem_port(name.c_str()));
			mem_ports.back()->register_b_transport(this, &memory::func);
		}
	}
	~memory(){}

	void func(tlm::tlm_generic_payload &trans, sc_time &delay){
		cmd_extension *cmd;
		trans.get_extension(cmd);
		if(!cmd){
			return;
		}
		auto data_cmd_cast = std::dynamic_pointer_cast<data_command>(cmd->cmd);
		if(!data_cmd_cast){
			return;
		}
		auto data_read_bytes = std::dynamic_pointer_cast<data_read<uint8_t>>(data_cmd_cast);
		if(data_read_bytes){
			std::cout<<"read bytes\n";
			data_read_bytes->data = this->read<uint8_t>(data_read_bytes->addrs);
			return;
		}
		auto data_read_ints = std::dynamic_pointer_cast<data_read<int>>(data_cmd_cast);
		if(data_read_ints){
			std::cout<<"read ints\n";
			data_read_ints->data = this->read<int>(data_read_ints->addrs);
			return;
		}
		auto data_write_bytes = std::dynamic_pointer_cast<data_write<uint8_t>>(data_cmd_cast);
		if(data_write_bytes){
			std::cout<<"write bytes\n";
			write(data_write_bytes->data, data_write_bytes->addrs);
			return;
		}
	}

	template<class Type, class Addrs>
	xt::xarray<Type> read(xt::xexpression<Addrs> &addrs_expr){
		Addrs &addrs = addrs_expr.derived_cast();
		auto flatten_view = xt::reshape_view(bytes, {-1});
		auto read_bytes = xt::eval(xt::view(flatten_view, xt::keep(addrs)));
		read_bytes.reshape(addrs.shape());
		if(std::is_same<Type, uint8_t>::value){
			return read_bytes;
		}
		auto read_bytes_sh = read_bytes.shape();
		read_bytes_sh.back() /= sizeof(Type);
		xt::xarray<Type> read_type = xt::empty<Type>(read_bytes_sh);
		memcpy(read_type.data(), read_bytes.data(), read_bytes.size());
		return read_type;
	}

	template<class Type, class Addrs>
	void write(xt::xexpression<Type> &data_expr,
			xt::xexpression<Addrs> &addrs_expr)
	{
		Type &data = data_expr.derived_cast();
		Addrs &addrs = addrs_expr.derived_cast();
		auto flatten_view = xt::reshape_view(bytes, {-1});
		auto wr_view =xt::view(flatten_view, xt::keep(addrs)); 
		if(std::is_same<typename Type::value_type, uint8_t>::value){
			wr_view = data;
		}
	}
};
xt::xarray<uint8_t> memory::bytes = {};

class reader_writer:public sc_module{
	size_t word_size, words, ops;
public:
	SC_HAS_PROCESS(reader_writer);
	using rw_port = tlm_utils::simple_initiator_socket<reader_writer>;
	rw_port port;

	sc_in<bool> clk_in;

	reader_writer(sc_module_name name, size_t word_size, size_t words)
		:sc_module(name),
		word_size(word_size),
		words(words),
		port("rw"),
		clk_in("rw_clk_in"),
		ops(0)
	{
		//bind function to signals
		SC_THREAD(func);
		//make module sensitive to positive clock
		sensitive<<clk_in.pos();
	}

	void func(){
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<int> dist(0, words-1);

		while(true){
			wait();
			std::cout<<sc_time_stamp()<<"in rw\n";

			tlm::tlm_generic_payload trans;

			auto data_wr= std::make_shared<data_write<uint8_t>>();
			auto rnd = dist(gen); 
			auto addrs = xt::arange<uint8_t>(rnd*word_size, (rnd+1)*word_size);

			data_wr->data = xt::arange<uint8_t>(ops, ops+word_size);
			data_wr->addrs = addrs;
			cmd_extension *cmd = new cmd_extension(data_wr);
			trans.set_extension(cmd);

			sc_time tm;

			std::cout<<"writing bytes:"<<data_wr->data<<"\n";
			std::cout<<"to:"<<data_wr->addrs<<"\n";
			port->b_transport(trans, tm);
			delete cmd;

			auto data_read_bytes = std::make_shared<data_read<uint8_t>>();
			data_read_bytes->addrs = addrs;
			std::cout<<"reading bytes from:"<<addrs<<"\n";
			cmd = new cmd_extension(data_read_bytes);
			trans.set_extension(cmd);
			port->b_transport(trans, tm);
			std::cout<<"result:"<<data_read_bytes->data<<"\n";
			delete cmd;

			auto data_read_int = std::make_shared<data_read<int>>();
			data_read_int->addrs = addrs;
			std::cout<<"reading int from:"<<addrs<<"\n";
			cmd = new cmd_extension(data_read_int);
			trans.set_extension(cmd);
			port->b_transport(trans, tm);
			std::cout<<"result:"<<data_read_int->data<<"\n";

			this->ops++;
		}
	}
	size_t get_ops(){
		return ops;
	}
};
