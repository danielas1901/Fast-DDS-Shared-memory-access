Notes on the created testing program. as well as the functionality of the shared memory transportation in eprosima fastdds.

this program has been tested on one of fastdds's example programs HelloWorldSharedMem, and serves as more of a proof of concept rather than an active use case.

The program inherits source code from eprosima fastdds to quickly recreated object types in order to be use in the program, this also requires that the boost library for C++ be installed on the machine.

Using the LogInfo feature on fastdds, it was determined the 'main/global' port of the application had a port id of 7400. Future iterations of this program will need to predetermine the port id for practicality.

the port name consists of three pieces of info, the domain name, a constant string "_port"_ and the port id. From the logging information, the domain name appeared to be a constant string "fastrtps".

a port object, seen in SharedmemGlobal.hpp, primarily contains: a port segment, a port node, and a ring buffer. 

Segments are created and handled by the inclusion of the boost library in fastdds. These boost managed segments are wrapped into a class "SharedMemSegment" which is copied into the programs source files. A port segment holds a struct named PortNode.

A PortNode is a struct that is used by the port to hold critical information about the functionality of the port (id, number of listeners, other status variables), within the struct is another named ListenerStatus, one of the PortNode fields is an array of these ListenerStatus structs. Each listener (explained further down) that is registered to the port can have their information identified from these ListenerStatus structs. 

The ring buffer is a class type used for buffering data between multiple producers and consumers. the class itself is generic, but for the purpose of the program the ring buffer will hold data in the form of a struct named BufferDescriptor. A BufferDescriptor acts as a pointer to a buffer (space allocated in a participants segment such as a message or some form of data being send to other participants), pointers can't be used in shared memory transport, as segments need to be mapped to each processes memory space. instead a buffer descriptor is used, which contains information on the location of a specific buffer in a participants segment using the segment id (similar to the port id used for a port segment), and an offset

Using the port name given as input, in this case the main global port will be used, a SharedMemSegment shared pointer is created, this pointer now points to the port segment, which as stated earlier, contains the PortNode. Using the boost function find(), the PortNode can be obtained. The find function requires that we know the type and the name of the object that we are looking for. The PortNode struct has been copied over to the programs Source Files as apart of SharedMemGlobal.hpp file. The names of PortNode's follows a simple systematic naming scheme, a constant string "port_node_abi" + a number referring to the current abi version (Not sure what abi is) which in this case is 5. Should the find function return the PortNode, access is now availiable to its fields.

The Port class in SharedMemGlobal contains a contructor requiring the port segment and a PortNode. With this in mind, a shared pointer is made to the port object using the constructor using the already obtained port segment and PortNode.

The Ring buffer object inside a Port object contains a subclass listed as "Listener", multiple Listeners can be created on the Ring buffer and are used to determine when messages are available (BufferDescriptors in this senario). These messages are seen as another subclass called "Cell", it is a generic class with whatever datatype being used (once again, BufferDescriptors) being set to the type of the Cell's data field.

With a shared pointer to the Port object, a listener must be created and registered onto the Port's Ring buffer in order to move forward with obtaining BufferDescriptors. The "create_listener" function from the port object can achieved both of these tasks. This function require an index number in order to update the PortNode to what listeners are being used via the ListenerStatus struct mentioned above. The current program uses a pretermined index of 2 based off of knowledge in how the example program works (we know there are only 2 other listeners: 0 and 1). Future iterations of the software must calculate the first available index by looping through every index number and checking the "is_in_use" field in the ListenerStatus struct within the PortNode. 

With a Listener registerd to the Ring Buffer of the port, a Ring buffer Cell object pointer is initialized (null to start with). The Listener contains a "head" function which returns a Cell object from the Ring Buffer. This function is utilized in a while loop that sets the empty Cell variable to the Cell obtained from the head function, but will only move on once it obtains a non-nullptr Cell value.

As mentioned earlier, the data field in the cell object is of type BufferDescriptor, it is then possible to make a variable of type BufferDescriptor thats hold the BufferDescriptor from the data field of the Cell object. From the BufferDescriptor the segment id of the participant is pulled and then used to create the segment name (constant fastrtps_ + segment id). Using the same function to obtain the port segment, the participant segment can now open for access in the form of a shared pointer. 

The segments of the participant function slightly differently from the segments of the ports. While the port segment contains only a PortNode, a participant segment contains a different struct known as a BufferNode (seen in DataTypes/BufferNode.hpp). The BufferNode struct is what identifies the message within the segment in the form of a data offset and a datasize. As with BufferDescriptors, pointers cannot be used to identify data as segments need to be remapped into a processes address space, meaning an offset and size must be used to locate the message. 

The BufferNode is located within the segment by using the offset variable from the BufferDescriptor's field "buffer_node_offset". Finally, a pointer to the data of type void* which is set to the address of the start of the data message is created by using the segments "get_address_from_offset" function (buffer node -> data_offset field as param). The final for loop prints out the data message in hex form (similar to a pcap file dump).

As of now, the program can only obtain 1 buffer descriptor from the Ring Buffer at a time, in order to monitor multiple participants at once, changes must be made to the program that will allow the iteration through a Ring Buffer in order to obtain multiple segment id's.

As mentioned at the start, this program serves as a proof of concept which shows that outside access of a shared memory transport is possible. More work will need to be done to allow software to become more practical (i.e. fuzzing).