/*#include<stack>
#include<atomic>
#include<memory>
template<typename T>
class LockFreeStack_atomic{
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<std::shared_ptr<T>> next;
        node(T const& data):data(std::make_shared<T>(data)){};
    };
    
    std::stack<T> s;
    std::atomic<std::shared_ptr<T>>head;
    std::atomic<unsigned int> count;

public:
    LockFreeStack_atomic():head(nullptr),count(0){
        auto cmp_ex_weak=[](std::shared_ptr<node>& curr_head, std::shared_ptr<node> new_head)->bool {
            new_head->next=curr_head;
            return head.compare_exchange_weak(curr_head,new_head);
        };
        head.compare_exchange_weak(nullptr,nullptr,std::memory_order_relaxed,cmp_ex_weak);
    }

    void push(T const& data){
        std::shared_ptr<node> new_node=std::make_shared<node>(data);
        while(true){
            new_node->next=head.load();
            while(!head.compare_exchange_weak(new_node->next,new_node));
            ++count;
        }
    }

    std::shared_ptr<T> pop(){
        std::shared_ptr<T> old_head=head.load();
        while(old_head&&!head.compare_exchange_weak(old_head,old_head->next.load()));
        if(old_head){
            old_head->next=std::shared_ptr<node>();
            --count;
            return old_head->data;
        }
        return std::shared_ptr<T>();
    }

    bool empty(){
        return count.load()==0?true:false;
    }

    ~LockFreeStack_atomic(){
        while(pop());
    }
};

template<typename T>
class LockFreeStack{
private:
    struct node;
    struct counted_node_ptr{
        int external_count;
        node* ptr;
    };
    struct node{
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;

        node(T const& data):data(std::make_shared<T>(data)),internal_count(0){};
    };
    std::atomic<counted_node_ptr>head;

    void increase_head_count(counted_node_ptr& old_counter){
        counted_node_ptr new_counter;
        do{
            new_counter=old_counter;
            ++new_counter.external_count;
        }
        while(!head.compare_exchange_strong(old_counter,new_counter));
        old_counter.external_count=new_counter.external_count;
    }
public:
    ~LockFreeStack(){
        while(pop());
    }
    void push(T const& data){
        counted_node_ptr new_node;
        new_node.ptr=new node(data);
        new_node.external_count=1;
        new_node.ptr->next=head.load();
        while(!head.compare_exchange_weak(new_node.ptr->next,new_node));
    }
    std::shared_ptr<T> pop(){
        counted_node_ptr old_head=head.load();
        while(1){
            increase_head_count(old_head);
            node* const ptr=old_head.ptr;
            if(!ptr)
                return std::shared_ptr<T>();
            if(head.compare_exchange_strong(old_head,ptr->next)){
                std::shared_ptr<T> res;
                res.swap(ptr->data);  // 4
                int const count_increase=old_head.external_count-2;  // 5
                if(ptr->internal_count.fetch_add(count_increase)==  // 6
                    -count_increase){
                    delete ptr;
                }
                return res;
            }
            else if(ptr->internal_count.fetch_sub(1)==1)
                delete ptr;
        }
    }
};*/