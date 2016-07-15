#include "te_service_impl.h"
#include "info_manager.h"
#include "task.h"

te_rpc::Task toRpc( const tasks::task_ptr t )
{
    te_rpc::Task tmp;

    tmp.set_pid( t->pid() );
    tmp.set_name( t->name() );
    tmp.set_path_name( t->path_name() );
    for( auto arg : t->argv() )
    {
        tmp.add_args( arg );
    }
    auto& envs = *tmp.mutable_envs();
    for( auto env : t->envv() )
    {
        envs[env.first] = env.second;
    }

    return tmp;
}

TEServiceImpl::~TEServiceImpl()
{
}

TEServiceImpl::TEServiceImpl() : m_im( new info_manager() )
{
}

grpc::Status TEServiceImpl::ActiveTasks(
    grpc::ServerContext* context, const te_rpc::InfoType* request,
    grpc::ServerWriter<te_rpc::Task>* writer )
{
    auto tasks = m_im->active_tasks();

    for( auto& tp : *tasks )
    {
        writer->Write( toRpc( tp.second ) );
    }
    return grpc::Status::OK;
}
