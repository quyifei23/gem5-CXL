#include <iostream>
#include <list>
#include <map>
#include <unistd.h>
typedef int* Addr;
typedef unsigned int Data; // need change

struct transaction_s;
typedef transaction_s transaction_t;

struct journal_s;
typedef journal_s journal_t;

struct journal_superblock_s;
typedef journal_superblock_s journal_superblock_t;

struct waitqueue_s;
typedef waitqueue_s waitqueue_t;

struct journal_header_s;
typedef journal_header_s journal_header_t;
struct handle_s
{
    transaction_t *h_transaction; // 本原子操作属于哪个transaction
    // int h_buffer_credits;         // 本原子操作包含几个磁盘块数
    unsigned int h_sync;          // 处理完该原子操作以后，立即将所属的transaction提交
};
typedef handle_s handle_t;

enum state {
    T_RUNNING,
    T_LOCKED,
    T_FLUSH,
    T_COMMIT,
    T_COMMIT_RECORD,
    T_FINISHED
};
struct transaction_s
{
    journal_t *t_journal; // 本事务属于哪个journal
    unsigned int t_tid;   // 本事务的数据号

    // 事务的状态：
    enum state t_state;

    unsigned int t_log_start;                  // 本事务从哪个日志块开始
    int t_n_buffer;                            // 本事务操作包含几个磁盘块数
    struct journal_head *t_reserved_list;      // 本事务保留但是并未修改的缓冲区组成的双向链表
    struct journal_head *t_locked_list;        // 由提交时所有正在被写出的，被锁住的缓冲区组成的双向链表
    struct jounal_head *t_buffers;             // 元数据缓冲区链表
    struct journal_head *t_sync_datalist;      // 本事务被提交之前，需要被刷新到磁盘上的数据块组成的双向链表
    struct journal_head *t_forget;             // 被遗忘的缓冲区链表。当本事务提交后，该缓冲区的checkpoint项就没有用了
    struct journal_head *t_checkpoint_list;    // 本事务可被checkpoint之前，需要被刷新到磁盘上的所有缓冲区的链表。
    struct journal_head *t_checkpoint_oi_list; // checkpoint时，已经提交进行io操作的缓冲区链表
    struct journal_head *t_iobuf_list;         // 进行临时性IO的元数据缓冲区的链表
    struct journal_head *t_shadow_list;        // 被日志IO复制过的元数据缓冲区的链表
    // t_iobuf_list上的缓冲区始终与t_shadow_list上的缓冲区一一对应
    // 当一个元数据块缓冲区要被写到日志中时，数据会被复制一份，被放到新的缓冲区，然后放入t_iobuf_list中，而原来的缓冲区会进入t_shadow_list队列
    struct journal_head *t_log_list;  // 正在写入日志的起控制作用的缓冲区组成的链表
    pthread_spinlock_t t_handle_lock; // 保护handle的锁
    int t_updates;                    // 与本事务相关联的外部更新的次数，实际上是正在使用本事务的handle的数量
    // 当journal_start时递增，journal_stop时递减
    // 当t_updates==0时，表示没有handle正在使用本事务，此时处于一种可提交状态
    int t_outstanding_credits;          // 本事务预留的额度
    transaction_t *t_cpnext, *t_cpprev; // 可用于checkpoint队列上组成链表
    int i_handle_count;                 // 本事务包括多少handle_t

    //add
    std::list<handle_t *> handles;
    std::list<Addr> addresses;
    std::list<Data> datas;
};

struct journal_s
{
    unsigned long j_flags;           // journal的状态
    struct buffer_head *j_sb_buffer; // 指向日志超级块缓冲区
    journal_superblock_t *j_superblock;
    int j_format_version;
    pthread_spinlock_t j_state_lock;
    int j_barrier_count;                      // 有多少进程正在等待创建一个barrier_lock，这个变量由j_state_lock保护
    struct mutex *j_barrier;                  // 互斥锁
    transaction_t *j_running_transaction;     // 指向正在运行的事务
    transaction_t *j_committing_transaction;  // 指向正在提交的事务 
    transaction_t *j_checkpoing_transactions; // 仍在等待checkpoint操作的所有事务组成的循环队列
    // 一旦一个事务执行checkpoint完成，则从此队列中删除
    waitqueue_t *j_wait_transaction_locked; // 等待一个已经上锁的事务开始提交
    waitqueue_t *j_wait_log_space;          // 等待checkpoint完成以释放日志空间的等待队列
    waitqueue_t *j_wait_done_commit;        // 等待提交完成的等待队列
    waitqueue_t *j_wait_checkpoint;         // 等待checkpoint的队列
    waitqueue_t *j_wait_commit;             // 等待提交的队列
    waitqueue_t *j_wait_updates;            // 等待handle完成的队列
    struct mutex *j_checkpoint_mutex;       // 保护checkpoint队列的互斥锁
    unsigned int j_head;                    // journal中第一个未使用的块
    unsigned int j_tail;                    // journal仍在使用的最旧的块，这个值为0，则整个journal是空的
    unsigned int j_free;
    unsigned int j_first;
    unsigned int j_last;
    // 表示日志的范围是[j_first,j_last]，一旦文件系统格式化就被保存在超级块中的不变量
    struct block_device *j_dev;
    int j_blocksize;
    unsigned int j_blk_offset; // 本日志相对于设备块的偏移量
    struct block_device *j_fs_dev;
    unsigned int j_maxlen; // 磁盘上日志的最大块数
    pthread_spinlock_t j_list_lock;
    struct inode *j_inode;
    unsigned int j_tail_sequence;
    // 日志中最旧的事务的序号
    unsigned int j_transaction_sequence;
    // 下一个授权的事务的顺序号
    unsigned int j_commit_sequence;
    // 最近提交的transaction的顺序号
    unsigned int j_commit_request;
    // 最近相申请提交的transaction的编号。
    // 如果一个transaction想提交，则把自己的编号赋值给j_commit_request，
    // 然后kjournald会择机进行处理。
    unsigned int j_uuid[16];
    struct task_struct *j_task;
    // 本journal指向的内核线程
    int j_max_transaction_buffers;
    // 一次提交允许的最多的元数据缓冲区块数
    unsigned long j_commit_interval;
    struct timer_list *j_commit_timer;
    // 用于唤醒提交日志的内核线程的定时器
    pthread_spinlock_t j_revoke_lock;
    // 保护revoke 哈希表
    struct jbd_revoke_table_s *j_revoke;
    // 指向journal正在使用的revoke hash table
    struct jbd_revoke_table_s *j_revoke_table[2];
    struct buffer_head **j_wbuf;
    // 指向描述符块页面
    int j_wbufsize;
    // 一个描述符块中可以记录的块数
    pid_t j_last_sync_writer;
    void *j_private;
    // 指向ext3的superblock

    //add
    std::map<Addr,Data> log;

};

// 日志超级块在内存中的表现
// struct journal_superblock_s
// {
//     journal_header_t *s_header; // 用于表示本块是一个超级块
//     int s_blocksize;            // journal所以在设备的块大小
//     int s_maxlen;               // 日志的长度，包括多少个块
//     int s_first;                // 日志的开始块号，日志相当于一个文件，这里提到的开始块号是文件中的逻辑块号
//     int s_sequence;             // 日志中第一个期待的commit ID ，指的是日志中最旧的一个事务的ID
//     int s_start;                // 日志开始的块号，表示本次有效日志块的起点
// };

// 一个buffer_head对应一个磁盘块，而一个journal_head对应一个buffer_head。日志通过journal_head对缓冲区进行管理。
// struct journal_head
// {
//     struct buffer_head *b_bh;
//     int b_jcount;
//     unsigned b_jlist;
//     // 本journal_head在transaction_t的哪个链表上
//     unsigned b_modified;
//     // 标志该缓冲区是否以被当前正在运行的transaction修改过
//     char *b_frozen_data;
//     // 当jbd遇到需要转义的块时，
//     // 将buffer_head指向的缓冲区数据拷贝出来，冻结起来，供写入日志使用。
//     char *b_committed_data;
//     // 目的是防止重新写未提交的删除操作
//     // 含有未提交的删除信息的元数据块（磁盘块位图）的一份拷贝，
//     // 因此随后的分配操作可以避免覆盖未提交的删除信息。
//     // 也就是说随后的分配操作使用的时b_committed_data中的数据，
//     // 因此不会影响到写入日志中的数据。
//     transaction_t *b_transaction;
//     // 指向所属的transaction
//     transaction_t *b_next_transaction;
//     // 当有一个transaction正在提交本缓冲区，
//     // 但是另一个transaction要修改本元数据缓冲区的数据，
//     // 该指针就指向第二个缓冲区。

//     struct journal_head *b_tnext, *b_tprev;

//     transaction_t *b_cp_transaction;
//     // 指向checkpoint本缓冲区的transaction。
//     // 只有脏的缓冲区可以被checkpointed。

//     struct journal_head *b_cpnext, *b_cpprev;
//     // 在旧的transaction_t被checkpointed之前必须被刷新的缓冲区双向链表。
// };

// struct journal_header_s
// {
//     int h_magic;
//     int h_blocktype;
//     int h_sequence; 
// };

//原子操作和事务的创建和删除
handle_t * new_handle() {
    handle_t *handle = new handle_t();
    handle->h_sync = 0;
    return handle;
}

void delete_handle(handle_t *handle) {
    delete handle;
}

transaction_t *new_transaction() {
    transaction_t *transaction = new transaction_t();
    return transaction;
}
void delete_transaction(transaction_t *transaction) {
    delete transaction;
}

//JBD2的操作接口

journal_t *journal_init();//初始化日志系统

journal_t *journal_init() {
    journal_t * journal = new journal_t();
    transaction_t *transaction = new_transaction();
    transaction->t_state = T_RUNNING;
    transaction->t_journal = journal;
    journal->j_running_transaction = transaction;
    return journal;
}

// int journal_load(journal_t *journal);//读取并恢复已有日志（如果存在）

// int journal_destroy(journal_t *journal);//销毁内存中日志系统的信息
void journal_destroy(journal_t *journal) {
    delete journal;
}

//JBD2的事务和原子操作接口

handle_t *journal_start(journal_t *journal);//在当前事务中开始一个新的原子操作


handle_t *journal_start(journal_t *journal) {
    handle_t *handle = new_handle();
    if(!journal->j_running_transaction) {
        transaction_t *transaction = new_transaction();
        transaction->t_state = T_RUNNING;
        transaction->t_journal = journal;
        journal->j_running_transaction = transaction;
    }
    while(journal->j_running_transaction->t_state!=T_RUNNING) {
        sleep(1);
    }
    handle->h_transaction = journal->j_running_transaction;
    journal->j_running_transaction->handles.push_back(handle);
    return handle;
}


// int journal_get_write_access(handle_t *handle, buffer_head *bh);//通知JBD2即将修改缓冲区bh中的元数据
//在这里我们修改为：通知JBD2即将修改地址为addr中的数据；
void journal_get_write_access(handle_t *handle, Addr addr, Data data) {
    handle->h_transaction->addresses.push_back(addr);
    handle->h_transaction->datas.push_back(data);
}

// int journal_get_create_access(handle_t *handle, buffer_head *bh);//通知JBD2即将使用一个新的缓冲区

// int journal_dirty_metadata(handle_t *handle, buffer_head *bh);//通知JBD2该缓冲区包含脏元数据

// int journal_stop(handle_t *handle);//结束一个原子操作
//将该handle与所属的transaction断开联系，如果该原子操作是同步的，则立即将所属的transaction提交。最后将该handle删除。
void journal_flush(transaction_t *transaction);
void journal_stop(handle_t *handle) {
    if(handle->h_sync==1) {
        //提交所属事务
        journal_flush(handle->h_transaction);
        return ;
    }
    handle->h_transaction->handles.pop_front();
    delete_handle(handle);
}

// int journal_commit(journal_t *journal);//提交当前事务

//将当前事务写入日志
void journal_flush(transaction_t *transaction) {
    transaction->t_state = T_FLUSH;//更改正在运行的状态为写入日志状态
    while(!transaction->addresses.empty()) {
        transaction->t_journal->log[transaction->addresses.front()] = transaction->datas.front();
        transaction->addresses.pop_front();
        transaction->datas.pop_front();
    }
    transaction->t_journal->j_running_transaction = NULL;
    delete_transaction(transaction);
}





