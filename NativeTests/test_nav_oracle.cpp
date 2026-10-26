#include "../Core/NavS3.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <array>
#include <string>
using namespace xone::l01;
int main(){try{
    NavS3 nav;if(!nav.initializeDefault())throw std::runtime_error("default");
    std::ifstream f("Fixtures/SOURCE_NAV_DEFAULT_52.csv");if(!f)throw std::runtime_error("oracle fixture not found");
    std::string line;std::getline(f,line);
    std::array<bool,52> seen{};int count=0;
    while(std::getline(f,line)){
        if(line.empty())continue;
        std::istringstream in(line);std::string a[6];
        for(auto& v:a){if(!std::getline(in,v,','))throw std::runtime_error("malformed fixture");}
        const int j=std::stoi(a[0]),addrIndex=std::stoi(a[1]),posIndex=std::stoi(a[2]);
        if(j<4||j>=56||seen[static_cast<std::size_t>(j-4)])throw std::runtime_error("invalid physical");
        seen[static_cast<std::size_t>(j-4)]=true;
        if(addrIndex!=320+2*(j-4)||posIndex!=addrIndex+1)throw std::runtime_error("source offsets");
        const auto r=nav.active().route(j);
        if(r.address!=std::stoi(a[3])||r.position!=std::stoi(a[4]))throw std::runtime_error("source @zd parity mismatch for physical "+std::to_string(j));
        ++count;
    }
    if(count!=52)throw std::runtime_error("not 52 fixture rows");
    std::cout<<"PASS 52/52 literal @zd default-page route fixtures (independent formula)\n";
    return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
