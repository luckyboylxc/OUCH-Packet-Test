#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <unordered_map>

using namespace std;

enum MessageType { accepted = 0, systemEvent = 1, replaced = 2, cancelled = 3, executed = 4 };

unordered_map<char, int> expectedSize{ {'S', 13}, {'A', 68}, {'U', 82}, {'E', 43}, {'C', 31} };
uint32_t swap_uint32( unsigned int val );

unsigned int totalSize = 0;


class Stream {

    public:

    int accepted{0};
    int systemEvent{0};
    int replaced{0};
    int cancelled{0};
    int executed{0};
    int executedShares{0};

        void updateEvent( char messageChar ) {

            switch( messageChar ) {

                case 'A': accepted += 1;
                case 'S': systemEvent += 1;
                case 'U': replaced += 1;
                case 'C': cancelled += 1;
                case 'E': executed += 1;

            } 


        }

        void printStream( const int& streamID ) {

            cout << "Stream \t" << streamID << "\n";
            cout << "Accepted: \t" << accepted << " messages\n";
            cout << "System Event: \t" << systemEvent << " messages\n";
            cout << "Replaced: \t" << replaced << " messages\n";            
            cout << "Cancelled: \t" << cancelled << " messages\n";
            cout << "Executed: \t" << executed << " messages\n";
            cout << "Executed Shares: \t" << executedShares << "\n";
        
        }

        void updateExecutedShares(int shares) {

            executedShares += swap_uint32( shares );

        }

};

class Total {

    int accepted{0};
    int systemEvent{0};
    int replaced{0};
    int cancelled{0};
    int executed{0};
    int executedShares{0};

    public:

        void updateTotal( Stream& packetStream) {

            accepted += packetStream.accepted;
            systemEvent += packetStream.systemEvent;
            replaced += packetStream.replaced;
            cancelled += packetStream.cancelled;
            executed += packetStream.executed;
            executedShares += packetStream.executedShares;

            }

        void printTotal() {

            cout << "Total: \n";
            cout << "Accepted: \t" << accepted << " messages\n";
            cout << "System Event: \t" << systemEvent << " messages\n";
            cout << "Replaced: \t" << replaced << " messages\n";            
            cout << "Cancelled: \t" << cancelled << " messages\n";
            cout << "Executed: \t" << executed << " messages\n";
            cout <<"Executed Shares: \t" << executedShares << "\n";
        
        }

       
};

struct PacketStatus {

    int countUpdated{0};
    char type;

};

unordered_map<int, PacketStatus > partialPacket;
unordered_map<int, Stream> nPackets;

//! Byte swap unsigned short
uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

//! Byte swap unsigned int
uint32_t swap_uint32( unsigned int val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

void getPacketHeader( unsigned int* buffer, FILE* ifs ) {

    // function used to read the contents of file
        fread(&buffer[0], 2, 1, ifs);
        fread(&buffer[1], 4, 1, ifs );

        buffer[0] = swap_uint16( buffer[0] );
        buffer[1] = swap_uint32( buffer[1] );

        totalSize += 6;
        totalSize += buffer[1];

        //cout << "Stream ID: " << buffer[0] << "\n" << "Packet Len: " << buffer[1];

}

void printPartialPacket() {

    for( auto& partialPacketPair: partialPacket ) {
        cout << "Stream: " << partialPacketPair.first << "\t Message type: "<<partialPacketPair.second.type <<"\t Message Size: "<<partialPacketPair.second.countUpdated;
        cout << endl;
    }

}

void parsePacket( FILE* ifs, unsigned int* buffer, int packetSize, int sID  ) {
    //cout << "Current Stream ID: " << sID << ".Packet Len: " << packetSize<<"\n";
    //printPartialPacket();

    if( partialPacket.find( sID ) != partialPacket.end() ) {

        if( partialPacket[ sID ].countUpdated <= 3 ) {

            if( partialPacket[ sID ].countUpdated + packetSize > 3 ) {

                char val;
                fseek( ifs, 3 - partialPacket[sID].countUpdated, SEEK_CUR );
                fread( &val, 1, 1, ifs );
                nPackets[ sID ].updateEvent( val );
                partialPacket[ sID ].type = val;  

                if( val == 'E' ) {
                    partialPacket[sID].type = 'E';

                    if( partialPacket[sID].countUpdated + packetSize > 26 ) {

                        fseek( ifs, 22, SEEK_CUR );
                        unsigned int shares;
                        fread( &shares, 4, 1, ifs );
                        nPackets[ sID ].updateExecutedShares( shares );
                        fseek( ifs, packetSize - ( 4 - partialPacket[sID].countUpdated + 26 ), SEEK_CUR );

                        if( partialPacket[sID].countUpdated + packetSize == expectedSize['E'] ) {

                            partialPacket.erase(sID);

                        }
                        else {

                            partialPacket[sID].countUpdated += packetSize;

                        }

                    }
                    else {

                        fseek( ifs, packetSize - ( 4 - partialPacket[sID].countUpdated), SEEK_CUR );
                        partialPacket[sID].countUpdated += packetSize;                        

                    }

                }
                else {

                    auto type = partialPacket[sID].type;
                    fseek( ifs, packetSize - ( 4 - partialPacket[sID].countUpdated ) ,SEEK_CUR );

                    if( partialPacket[sID].countUpdated + packetSize == expectedSize[ val ] ) {

                        partialPacket.erase(sID);

                    }
                    else {

                        partialPacket[sID].countUpdated += packetSize;

                    }

                    

                }

            }
            else {
                
                fseek( ifs, packetSize, SEEK_CUR );
                partialPacket[sID].countUpdated += packetSize;

            }

        }
        else {

            if( partialPacket[sID].type == 'E' ) {

                if( partialPacket[sID].countUpdated < 26 ) {
                    
                    if( partialPacket[sID].countUpdated + packetSize > 26 ) {
                        
                        fseek( ifs, 26 - partialPacket[sID].countUpdated, SEEK_CUR );
                        unsigned int shares;
                        fread( &shares, 4, 1, ifs );
                        nPackets[sID].updateExecutedShares( shares );
                        
                        fseek( ifs, packetSize - ( 30 - partialPacket[sID].countUpdated ), SEEK_CUR );

                        if( partialPacket[sID].countUpdated + packetSize == expectedSize['E'] ) {

                            partialPacket.erase( sID );

                        }
                        else {

                            partialPacket[sID].countUpdated += packetSize;

                        }

                    }
                    else {

                        fseek( ifs, packetSize, SEEK_CUR );
                        partialPacket[sID].countUpdated += packetSize;

                    }

                }
                else {
                    
                    fseek( ifs, packetSize, SEEK_CUR );

                    if( partialPacket[sID].countUpdated + packetSize == expectedSize[ 'E' ] ) {

                        partialPacket.erase(sID);

                    }
                    else {

                        partialPacket[sID].countUpdated += packetSize;

                    }

                }

            }
            else {

                fseek( ifs, packetSize, SEEK_CUR );

                if( partialPacket[sID].countUpdated + packetSize == expectedSize[ partialPacket[sID].type ] ) {

                    partialPacket.erase(sID);

                }
                else {

                    partialPacket[sID].countUpdated += packetSize;

                }

            }

        }
    }
    else {

        if( packetSize <= 3 ) {

            partialPacket[sID] = PacketStatus{ packetSize, '/' };
            fseek( ifs, packetSize, SEEK_CUR );

        }
        else {

            char val;
            fseek( ifs, 3, SEEK_CUR );
            fread( &val, 1, 1, ifs );
            nPackets[ sID ].updateEvent( val );            

            if( val == 'E' ) {

                if( packetSize > 26 ) {

                    fseek( ifs, 22, SEEK_CUR );
                    unsigned int shares;
                    fread( &shares, 4, 1, ifs );
                    fseek( ifs, packetSize - 30, SEEK_CUR );

                    nPackets[ sID ].updateExecutedShares( shares );

                    if( packetSize != expectedSize['E'] ) {

                        partialPacket[sID] = { packetSize, 'E' };
                    }

                }
                else {

                    fseek( ifs, packetSize - 4, SEEK_CUR );
                    partialPacket[sID] = { packetSize, 'E' };

                }

            }
            else {

                fseek( ifs, packetSize - 4, SEEK_CUR );

                if( packetSize != expectedSize[val] ) {

                    partialPacket[sID] = { packetSize, val };

                }

            }

        }

    }

}

void parsePackets( FILE* ifs ) {

    unsigned int buffer[2];

    while( ftell(ifs) < 5702889 ) {

        getPacketHeader( buffer, ifs );
        parsePacket( ifs, buffer, buffer[1], buffer[0] );

        buffer[0] = buffer[1] = 0;

    }


}



void printOutput() {

    for( auto& streamPair: nPackets ) {

        streamPair.second.printStream( streamPair.first );
        cout << endl;

    }

}


int main() {

    FILE* ifs = fopen( "OUCHLMM2.incoming.packets", "rb" );

    // Define a vector to store the strings received from input
    unsigned int buffer[2];
    Total summaryTotal;
    
    parsePackets( ifs );
    fclose(ifs);
    printOutput();

    
    for( auto& streamPair: nPackets ) {

        summaryTotal.updateTotal(streamPair.second);
    }

    summaryTotal.printTotal();
    
    //cout<<"Total file size: " <<totalSize<<endl;

    return 0;
}