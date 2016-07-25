#include "rpc.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <iostream>
#include <memory>
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;

using te_rpc::TEDataProvider;
using te_rpc::InfoType;
using te_rpc::Task;

class TEClient
{
public:
    TEClient( std::shared_ptr<Channel> channel )
        : stub_( TEDataProvider::NewStub( channel ) )
    {
    }

    void Work()
    {
        InfoType request;
        request.set_type( te_rpc::InfoType_Type_SHORT );
        request.set_slice( te_rpc::InfoType_Slice_NEW );

        InfoType t;

        ClientContext context;
        auto          stream = stub_->ActiveTasks( &context, request );

        Task task;
        while( stream->Read( &task ) )
        {
            std::cout << task.name() << "[";
            // for( auto env : task.envs() )
            //{
            // std::cout << env.first << ":" << env.second << std::endl;
            //}
            // std::cout << "]" << std::endl << "[";
            // for( auto arg : task.args() )
            //{
            // std::cout << arg << std::endl;
            //}
            std::cout << "]" << std::endl;
        }
    }

private:
    std::unique_ptr<TEDataProvider::Stub> stub_;
};

int main( int argc, char *argv[] )
{
    TEClient te( grpc::CreateChannel( "localhost:50051",
                                      grpc::InsecureChannelCredentials() ) );
    te.Work();

    return 0;
}
