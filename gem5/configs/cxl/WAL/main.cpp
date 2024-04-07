#include "JBD.h"
int main()
{
    int a = 0, b = 0, c = 0, d = 0;
    journal_t *journal = journal_init();

    handle_t *handle = journal_start(journal);
    journal_get_write_access(handle,&a,10);
    journal_stop(handle);
    // std::cout << "pass 1" <<std::endl;

    handle = journal_start(journal);
    journal_get_write_access(handle,(Addr)&b,20);
    journal_stop(handle);
    // std::cout << "pass 2" <<std::endl;

    handle = journal_start(journal);
    handle->h_sync = 1;
    journal_get_write_access(handle,(Addr)&c,30);
    journal_stop(handle);
    // std::cout << "pass 3" <<std::endl;

    handle = journal_start(journal);
    handle->h_sync = 1;
    journal_get_write_access(handle,(Addr)&d,40);
    journal_stop(handle);

    std::cout<<"PRINT LOG:"<<std::endl;

    for (auto it = journal->log.begin(); it != journal->log.end(); ++it)
    {
            std::cout <<it->first<<": " <<it->second<<std::endl;
    }
    journal_destroy(journal);
    return 0;
}