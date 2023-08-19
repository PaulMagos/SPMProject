//
// Created by p.magos on 8/19/23.
//
#include <cstddef>
#include <string>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace bip = boost::interprocess;

int main(){
    std::string filename = "./data/TestFiles/test1.txt";
    bip::file_mapping mapping(filename.c_str(), bip::read_only);
    bip::mapped_region mapped_rgn(mapping, bip::read_only);
    char const* const mmaped_data = static_cast<char*>(mapped_rgn.get_address());
    std::size_t const mmap_size = mapped_rgn.get_size();
}