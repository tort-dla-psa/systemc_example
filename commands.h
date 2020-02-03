#pragma once
#include <tlm.h>
#include <memory>
#include <vector>
#include <xtensor/xarray.hpp>

class command{
public:
	virtual ~command(){}
};

class data_command:public command{
public:
	xt::xarray<size_t> addrs;
};

template<class Type>
class data_write:public data_command{
public:
	xt::xarray<Type> data;
};

template<class Type>
class data_read:public data_command{
public:
	xt::xarray<Type> data;
};

class cmd_extension:public tlm::tlm_extension<cmd_extension>{
public:
	std::shared_ptr<command> cmd;
	cmd_extension(std::shared_ptr<command> cmd){
		this->cmd = cmd;
	}

	tlm::tlm_extension_base* clone()const{
		return new cmd_extension(*this);
	}

	void copy_from(const tlm::tlm_extension_base &rhs){
		*this = static_cast<const cmd_extension&>(rhs);
	}
};
