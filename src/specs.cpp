/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Built-in protocol specifications, ported from specs.py of the original
 * Python tool by Luis MartinGarcia (spec strings are identical).
 *
 * This file is part of Protocol, distributed under the GNU GPL v3 license.
 */
#include "specs.hpp"

namespace protocol {

const std::map<std::string, std::string>& builtin_protocols() {
    static const std::map<std::string, std::string> protocols = {
        {"8021q", "Destination Address:48,Source Address:48,TPID (0x8100):16,PCP:3,D:1,VLAN ID:12,EtherType:16,Payload:96?bits=48"},
        {"cotp_cr", "Length:8,PDU Type:8, Destination Reference:16, Source Reference:16,Class/Options:8,Param. Code:8,Param. Length:8,Param.:88"},
        {"cotp_dr", "Length:8,PDU Type:8, Destination Reference:16, Source Reference:16,Cause:8"},
        {"cotp_dt", "Length:8,PDU Type:8,Num. & LDU:8?bits=24"},
        {"dhcp", "Opcode:8,Hardware Type: 8,HW Addr Len:8,Hop Count:8,Transaction ID:32,Number of Seconds:16,Flags:16,Client IP Addr:32,Your IP Addr: 32,Server IP Addr:32,Gateway IP Addr:32,Client Hardware Addr:128,Server Host Name:512,Boot Filename:1024"},
        {"dnp3", "Start:16,Length:8,Control:8,Destination Address:16,Source Address:16,CRC:16,User Data 1:128,CRC 1:16,User Data 2:112,CRC 2:16"},
        {"dot1q", "Destination Address:48,Source Address:48,TPID (0x8100):16,PCP:3,D:1,VLAN ID:12,EtherType:16,Payload:96?bits=48"},
        {"ethernet", "Destination Address:48,Source Address:48,EtherType:16,Payload:128?bits=48"},
        {"example", "Field4:4,Field4:4,Field8:8,Field16:16,Field32:32,Field64:64?bits=16,numbers=y,startchar=*,endchar=*,evenchar=-,oddchar=-,sepchar=|"},
        {"icmp", "Type:8,Code:8,Checksum:16,Message Body:64"},
        {"icmp-destination", "Type:8,Code:8,Checksum:16,Unused:32,Internet Header + 64 bits of Original Data Datagram:64"},
        {"icmp-echo", "Type:8,Code:8,Checksum:16,Identifier:16,Sequence Number:16,Data:64"},
        {"icmp-information", "Type:8,Code:8,Checksum:16,Identifier:16,Sequence Number:16"},
        {"icmp-parameter", "Type:8,Code:8,Checksum:16,Pointer:8,Unused:24,Internet Header + 64 bits of Original Data Datagram:64"},
        {"icmp-redirect", "Type:8,Code:8,Checksum:16,Gateway Internet Address:32,Internet Header + 64 bits of Original Data Datagram:64"},
        {"icmp-source", "Type:8,Code:8,Checksum:16,Unused:32,Internet Header + 64 bits of Original Data Datagram:64"},
        {"icmp-time", "Type:8,Code:8,Checksum:16,Unused:32,Internet Header + 64 bits of Original Data Datagram:64"},
        {"icmp-timestamp", "Type:8,Code:8,Checksum:16,Identifier:16,Sequence Number:16,Originate Timestamp:32,Receive Timestamp:32,Transmit Timestamp:32"},
        {"icmpv6", "Type:8,Code:8,Checksum:16,Message Body:64"},
        {"icmpv6-big", "Type:8,Code:8,Checksum:16,MTU:32,Invoking packet data (without exceeding minimum IPv6 MTU):64"},
        {"icmpv6-destination", "Type:8,Code:8,Checksum:16,Unused:32,Invoking packet data (without exceeding minimum IPv6 MTU):64"},
        {"icmpv6-echo", "Type:8,Code:8,Checksum:16,Identifier:16,Sequence Number:16,Data:64"},
        {"icmpv6-nadv", "Type:8,Code:8,Checksum:16,R:1,S:1,O:1,Reserved:29,Target Address:128,Options:64"},
        {"icmpv6-nsol", "Type:8,Code:8,Checksum:16,Reserved:32,Target Address:128,Options:64"},
        {"icmpv6-parameter", "Type:8,Code:8,Checksum:16,Pointer:32,Invoking packet data (without exceeding minimum IPv6 MTU):64"},
        {"icmpv6-radv", "Type:8,Code:8,Checksum:16,Cur Hop Limit:8,M:1,O:1,Reserved:6,Router Lifetime:16,Reachable Time:32,Retransmission Timer:32,Options:64"},
        {"icmpv6-redirect", "Type:8,Code:8,Checksum:16,Reserved:32,Target Address:128,Destination Address:128,Options:64"},
        {"icmpv6-rsol", "Type:8,Code:8,Checksum:16,Reserved:32,Options:64"},
        {"icmpv6-time", "Type:8,Code:8,Checksum:16,Unused:32,Invoking packet data (without exceeding minimum IPv6 MTU):64"},
        {"ip", "Version:4,IHL:4,Type of Service:8,Total Length:16,Identification:16,Flags:3,Fragment Offset:13,Time to Live:8,Protocol:8,Header Checksum:16,Source Address:32,Destination Address:32,Options:24,Padding:8"},
        {"ipv6", "Version:4,Traffic Class:8,Flow Label:20,Payload Length:16,Next Header:8,Hop Limit:8, Source Address:128, Destination Address:128"},
        {"modbus_tcp", "Transaction ID:16,Protocol ID:16,Length:16,Address:8,Function Code:8,Data:64"},
        {"profinet_rt", "Frame ID:16,User Data:80,Cycle Counter:16,Data Status:8,Transfer Status:8"},
        {"s7_data", "Return Code:8,Transport Size:8,Data Length:16"},
        {"s7_header", "Protocol ID:8,ROSCTR:8,Reserved:16,Request ID:16,Parameter Length:16,Data Length:16,Error Code (only ROSCTR 3):16,Function Code:8,Item Count:8?bits=16"},
        {"s7_item", "Var Type:8,Var Length:8,Syntax ID:8,Transport Size:8,Length:16,DB Number:16,Area:8,Address:24"},
        {"tcp", "Source Port:16,Destination Port:16,Sequence Number:32,Acknowledgment Number:32,Offset:4,Res.:4,Flags:8,Window:16,Checksum:16,Urgent Pointer:16,Options:24,Padding:8"},
        {"test", "Field_1:1,Field_4:4,Field_7:7,Field_10:10,Field_13:13,Field_16:16,Field_19:19,Field_22:22,Field_25:25,Field_28:28,Field_31:31,Field_34:34,Field_37:37,Field_40:40,Field_43:43,Field_46:46,Field_49:49,Field_52:52,Field_55:55,Field_58:58,Field_61:61,Field_64:64,Field_67:67,Field_70:70,Field_73:73,Field_76:76,Field_79:79,Field_82:82,Field_85:85,Field_88:88,Field_91:91,Field_94:94,Field_97:97,Field_100:100,Field_103:103,Field_106:106,Field_109:109,Field_112:112,Field_115:115,Field_118:118,Field_121:121,Field_124:124,Field_127:127"},
        {"tsap", "Type:8,Slot:5,Rack:3?bits=16"},
        {"udp", "Source Port:16,Destination Port:16,Length:16,Checksum:16"},
    };
    return protocols;
}

} // namespace protocol
