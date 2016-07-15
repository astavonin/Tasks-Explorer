#pragma once

#include <grpc++/grpc++.h>
#include <rpc.grpc.pb.h>
#include <memory>

class info_manager;

class TEServiceImpl final : public te_rpc::TEDataProvider::Service
{
public:
    TEServiceImpl();
    ~TEServiceImpl();

protected:
    grpc::Status ActiveTasks(
        grpc::ServerContext* context, const te_rpc::InfoType* request,
        grpc::ServerWriter<te_rpc::Task>* writer ) override;

private:
    std::unique_ptr<info_manager> m_im;
};
