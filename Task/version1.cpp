#include "SourceFiles/boostconfig.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>

#include <iostream>
#include <string>
#include <stdint.h>
#include <random>
#include <chrono>
#include <vector>
#include <mutex>
#include <array>
#include <thread>

#include "SourceFiles/SharedMemSegment.hpp"
#include "SourceFiles/SharedMemUUID.hpp"
#include "SourceFiles/RobustInterprocessCondition.hpp"
#include "SourceFiles/MultiProducerConsumerRingBuffer.hpp"
#include "SourceFiles/SharedDir.hpp"
#include "SourceFiles/RobustExclusiveLock.hpp"
#include "SourceFiles/RobustSharedLock.hpp"
#include "SourceFiles/SharedMemWatchdog.hpp"
#include "SourceFiles/SharedMemGlobal.hpp"

#include "DataTypes/BufferNode.hpp"

using namespace std;
using namespace boost::interprocess;

int main(int argc, char * argv[]){
	
	if (argc != 2){
       printf("Use proper format, Example: %s domain_name + '_port' + port_id\n", argv[0]);
       exit(1);
    }
	//take input as the name of the port segment, must be known beforehand in this version
	string port_segment_name = argv[1];
	
	static const uint32_t CURRENT_ABI_VERSION = 5;
	//make a port segment of the 'proper' type
	auto Port_Segment = std::shared_ptr<eprosima::fastdds::rtps::SharedMemSegment>(new eprosima::fastdds::rtps::SharedMemSegment(open_only, port_segment_name.c_str()));
	//port node
	eprosima::fastdds::rtps::SharedMemGlobal::PortNode *port_node = Port_Segment->get().find<eprosima::fastdds::rtps::SharedMemGlobal::PortNode>(("port_node_abi" + std::to_string(CURRENT_ABI_VERSION)).c_str()).first;
	cout << "Domain Name: " << port_node->domain_name << endl;	//works!
	cout << "Num Listeners: " << port_node->num_listeners << endl;
	cout << "Healthy check timeout: " << port_node->healthy_check_timeout_ms << endl;
	cout << "Port wait timeout: " << port_node->port_wait_timeout_ms << endl;
	cout << "Max buffer descriptors: " << port_node->max_buffer_descriptors << endl;
	cout << "Waiting count: " << port_node->waiting_count << endl;
	//port pointer, works!
	std::shared_ptr<eprosima::fastdds::rtps::SharedMemGlobal::Port> port = std::make_shared<eprosima::fastdds::rtps::SharedMemGlobal::Port>(move(Port_Segment), port_node);
	uint32_t portid = port->port_id();
	cout << "Port id from port object: " << portid << endl;
	cout << "Port is port ok: " << port->is_port_ok() << endl;
	
	uint32_t fake_index = 2;
	auto new_lis = port->create_listener(&fake_index);
	//make port cell variable, use head() function from created listener, might need wait_pop function from port pointer
	eprosima::fastdds::rtps::MultiProducerConsumerRingBuffer<eprosima::fastdds::rtps::SharedMemGlobal::BufferDescriptor>::Cell* head_cell = nullptr;
	//wait for data
	while (nullptr == (head_cell = new_lis->head())){
		
	}
	
	//if empty, something went wrong
	if (!head_cell){
		cout << "no luck fellas" << endl;
	}
	
	//the buffer decrisptor is located in the data field of the cell
	eprosima::fastdds::rtps::SharedMemGlobal::BufferDescriptor bufferDes = head_cell->data();
	//get segment id and name
	string seg_name = "fastrtps_" + bufferDes.source_segment_id.to_string();
	cout << "Segment name: fastrtps_" << bufferDes.source_segment_id.to_string() << endl;
	
	//open the segment using set name (either the subscriber or publisher segment)
	auto Segment = std::make_shared<eprosima::fastdds::rtps::SharedMemSegment>(open_only, seg_name);
	//get the buffernode within the segment
	BufferNode* buffer_node = static_cast<BufferNode*>(Segment->get_address_from_offset(bufferDes.buffer_node_offset));
	
	//get the data size from the buffer node
	uint32_t data_size = buffer_node->data_size;
	cout << "BufferNode data size: " << data_size << endl;
	cout << "Buffernode offset: " << buffer_node->data_offset << endl;
	//get the data location from the buffer node...we're getting closer...
	void* data_ = Segment->get_address_from_offset(buffer_node->data_offset);
	
	//Print out the rtps message, end goal of prototype program has been reached!
	for (int i=0; i<data_size; i++){
		if (i%16==0)
		{
			printf("\n");
		}
		printf("%02x ", ((unsigned char*)data_)[i]);
	}
	
	return 0;
}