#include <iostream>
#include "task.h"
#include "info_manager.h"

int main(int argc, char *argv[])
{
    info_manager im;
    auto tasks = im.active_tasks();

    std::cout << "Active tasks count: " << tasks->size() << std::endl;

    for ( auto &tp : *tasks) {
        auto &t = tp.second;
        std::cout << t->pid() << "\t\t" << t->name() << "\t" << t->path_name() << std::endl;
    }

    return 0;
}
