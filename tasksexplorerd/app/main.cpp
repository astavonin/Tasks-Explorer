#include <grpc++/grpc++.h>
#include <rpc.grpc.pb.h>
#include <iostream>
#include "info_manager.h"
#include "task.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::Status;

using te_rpc::TEDataProvider;
using te_rpc::InfoType;
using te_rpc::Task;

class TEServiceImpl final : public TEDataProvider::Service
{
    Status ActiveTasks( ServerContext* context, const InfoType* request,
                        ServerWriter<Task>* writer ) override
    {
        info_manager im;
        auto         tasks = im.active_tasks();

        std::cout << "Active tasks count: " << tasks->size() << std::endl;

        for( auto& tp : *tasks )
        {
            auto& t = tp.second;
            Task tmp;
            tmp.set_pid( t->pid() );
            tmp.set_name( t->name() );
            tmp.set_path_name( t->path_name() );
            writer->Write( tmp );
        }
        return Status::OK;
    }
};

void RunServer()
{
    std::string        server_address( "0.0.0.0:50051" );
    TEServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort( server_address,
                              grpc::InsecureServerCredentials() );
    builder.RegisterService( &service );
    std::unique_ptr<Server> server( builder.BuildAndStart() );
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

int main( int argc, char* argv[] )
{
    RunServer();

    return 0;
}
