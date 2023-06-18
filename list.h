#include <iostream>
#include <vector>
#include <type_traits>
#include <list>
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <stdexcept>

template <size_t N>
struct StackStorage {
  char stack[N];
  size_t start_elem_index = 0;

  StackStorage(const StackStorage& stack) = delete;

  StackStorage operator=(const StackStorage& stack) = delete;

  StackStorage() = default;
};


template <typename T, size_t N>
struct StackAllocator {
  using value_type = T;

  StackStorage<N>* storage = nullptr;

  StackAllocator() {};

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& allocator) : storage(allocator.storage) {};

  StackAllocator(StackStorage<N>& storage) : storage(&storage) {};

  //StackAllocator& operator=(const StackAllocator& allocator) {
  //  StackAllocator copy = allocator;
  //  std::swap(this->storage, copy.storage);
  //  return *this;
  //}

  void deallocate(T*, size_t) {
  }

  template<typename U>
  struct Rebind {
    using other = StackAllocator<U, N>;
  };

  bool operator==(const StackAllocator& other) const{
    return this->storage == other.storage;
  }

  bool operator!=(const StackAllocator& other) const {
    return this->storage != other.storage;
  }

  T* allocate(size_t n) {
    auto start_index = storage->start_elem_index_;
    auto mod = reinterpret_cast<intptr_t>(storage->stack + storage->start_elem_index_) % alignof(T);
    if (mod != 0U) {
      storage->start_elem_index_ += alignof(T) - mod;
    }
    auto r = storage->stack + storage->start_elem_index_;
    storage->start_elem_index_ += sizeof(T) * n;
    if (storage->start_elem_index_ >= N) {
      storage->start_elem_index_ = start_index;
      throw std::runtime_error("");
    }
    return reinterpret_cast<T*>(r);
  }
};

template <typename T, typename Allocator = std::allocator<T>>
struct List {
 public:

  struct Node;
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode(BaseNode* prev_node = nullptr, BaseNode* next_node = nullptr) : prev(prev_node), next(next_node) {
    }
  };

  struct Node: BaseNode {
    T val;
    Node(const T& value) : val(value) {};

    template<typename... Args>
    Node(Args&&... args) : val(std::forward<Args>(args)...) {}
  };


  auto get_allocator() const {
    return allocatorNode;
  }

  size_t size() {
    return size_;
  }

  size_t size() const {
    return size_;
  }

  template<bool IsConstant, bool IsReversed>
  struct Iterator {
   private:
   public:
    BaseNode* val = nullptr;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::bidirectional_iterator_tag;

    Iterator(BaseNode *temp = nullptr): val(temp){}
    Iterator(const BaseNode *temp): val((BaseNode *)temp){}

    template<bool IsReversedElem>
    Iterator(const Iterator<IsConstant, IsReversedElem>& elem) : val(elem.data()) {}

    //template<bool IsReversedElem>
    //Iterator<true, IsReversedElem>() { return Iterator<true, IsReversedElem>(*this); }

    //Iterator& operator=(const Iterator& elem) {
    //  val = elem.val;
    //  return *this;
    //}

    Iterator operator++() {
      if (IsReversed) {
        val = val->prev;
      }
      else {
        val = val->next;
      }
      return *this;
    }

    Iterator operator--() {
      if (IsReversed) {
        val = val->next;
      }
      else {
        val = val->prev;
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator answer = *this;
      ++(*this);
      return answer;
    }

    Iterator operator--(int) {
      Iterator answer = *this;
      --(*this);
      return answer;
    }

    std::conditional_t<IsConstant, const T&, T&> operator*() {
      return reinterpret_cast<Node*>(val)->val;
    }

//     T& operator*() const {
//      return reinterpret_cast<Node*>(val)->val;
//    }

    std::conditional_t<IsConstant, const T*, T*> operator->()const {
      return &(val->val);
    }

    Node* data() const {
      return (Node*)val;
    }

    template<bool IsReversedElem>
    operator Iterator<true, IsReversedElem>() {
      return Iterator<true, IsReversedElem>(val);
    }

    template<bool  IsConstantElem, bool IsReversedElem>
    bool operator == (const Iterator< IsConstantElem, IsReversedElem>& oth) const {
      return val == oth.data();
    }

    template<bool  IsConstantElem, bool IsReversedElem>
    bool operator != (const Iterator< IsConstantElem, IsReversedElem>& oth) const {
      return val != oth.data();
    }

    Node* iterator_data() const {
      return val;
    }

    Iterator base() {
      return Iterator<IsConstant, !IsReversed>(val->next);
    }

    ~Iterator() noexcept = default;
  };

  using iterator = Iterator<false, false>;
  using const_iterator = Iterator<true, false>;
  using reverse_iterator = Iterator<false, true>;
  using const_reverse_iterator = Iterator<true, true>;

  template <typename U = T>
  void push_back(U&& val) {
    insert(end(), std::forward<U>(val));
  }
  template <typename U = T>
  void push_front(U&& val) {
    insert(begin(), std::forward<U>(val));
  }
  void pop_back() {
    erase(std::prev(end()));
  }
  void pop_front() {
    erase(begin());
  }

  void push_back_constructor() {
    Node* ptr = allocatorNode.allocate(1);
    std::allocator_traits<NodeAllocator>::construct(allocatorNode, ptr);
    ptr->prev = FakeNode.prev;
    ptr->next = FakeNode;
    FakeNode.prev->next = ptr;
  }

  iterator begin() {
    return FakeNode.next ? FakeNode.next : &FakeNode;
  }
  const_iterator begin() const {
    return const_iterator(FakeNode.next ? FakeNode.next : &FakeNode);
  }

  const_iterator end() const {
    return const_iterator(&FakeNode);
  }

  iterator end()  {
    return &FakeNode;
  }

  const_iterator cbegin() const {
    return const_iterator(FakeNode.next ? FakeNode.next : &FakeNode);
  }

  const_iterator cend() const {
    return &FakeNode;
  }

  reverse_iterator rbegin() const {
    return FakeNode.prev;
  }

  reverse_iterator rbegin() {
    return FakeNode.prev;
  }

  reverse_iterator rend() {
    return &FakeNode;
  }

  const_reverse_iterator crbegin() const {
    return FakeNode.prev;
  }

  const_reverse_iterator crend() const {
    return FakeNode;
  }
/*
  template <typename Y, typename... U>
  void emplace(Y ptr, U&&... val) {
    auto  elem3 = ptr.data();
    --ptr;
    auto  elem1 = ptr.data();
    auto  elem12 =  elem1;
    typename std::allocator_traits<Allocator>::template Rebind_alloc<Node> all(get_allocator());
    auto b_ptr = all.allocate(1);
     elem12 = new (b_ptr) Node(std::forward<U>(val)...);
     elem12->next =  elem3;
     elem12->prev =  elem1;
     elem3->prev =  elem12;
     elem1->next =  elem12;
    ++size_;
  }
*/
  template <typename Y, typename... U>
  void emplace(Y ptr, U&&... val) {
    auto  elem3 = ptr.data();
    auto  elem1 =  elem3->prev;
    if (! elem1)  elem1 =  elem3;
    auto  elem12 =  elem1;
    typename std::allocator_traits<Allocator>::template Rebind_alloc<Node> all(get_allocator());
    auto b_ptr = all.allocate(1);
    elem12 = new (b_ptr) Node(std::forward<U>(val)...);
    elem12->next =  elem3;
    elem12->prev =  elem1;
    elem3->prev =  elem12;
    elem1->next =  elem12;
    ++size_;
  }


  void erase(const_iterator iter) {
    Node* node = iter.data();
    node->next->prev = node->prev;
    node->prev->next = node->next;

    typename std::allocator_traits<Allocator>::template Rebind_alloc<Node> all(get_allocator());
    node->~Node();
    all.deallocate(node, 1);
    --size_;
  }

  template <typename Y, typename U = T>
  void insert(Y ptr, U&& val) {
    emplace(ptr, std::forward<U>(val));
  }

  explicit List(const Allocator& alloc = Allocator())
      :allocatorNode(static_cast<NodeAllocator>(alloc))
  {   }

  List (size_t size, const T& value) {
    try
    {
      for (size_t i; i < size; ++i) {
        push_back(value);
      }
    }
    catch (...)
    {
      this->~List();
    }
  }

  List(size_t size, const Allocator& allocator = Allocator())
      : allocatorNode(allocator) {
    try
    {
      for (size_t q = 0; q < size; ++q) {
        emplace(end());
      }
    } catch (...) {
      this->~List();
    }
  }

//  List(size_t n, Allocator  elem1) : NodeAllocator( elem1), Node({this, this}), size_(0) {
//    for (size_t q = 0; q < n; ++q) {
//      emplace(end());
//    }
//  }

//  List(size_t size, const T& value, const Allocator& allocator = Allocator())
//      : allocatorNode(allocator) {
//    for (size_t i; i < size; ++i) {
//      push_back(value);
//    }
//    for (Node* iter = FakeNode->next; iter != FakeNode; iter = iter->next) {
//      std::allocator_traits<NodeAllocator>::construct(allocatorNode, iter);
//    }
//  }

  List(const List& oth)
      : List(std::allocator_traits<Allocator>::select_on_container_copy_construction(oth.get_allocator())) {
    try {
      for (auto& w : oth) {
        push_back(w);
      }
    } catch (...) {
      this->~List();
    }
  }
  List(List&& oth)
      : List(oth.get_allocator())
  {
    auto FakeNode_tmp = oth.FakeNode;
    auto size_tmp = oth.size_;

    oth.FakeNode = BaseNode();
    oth.size_ = 0;

    FakeNode = FakeNode_tmp;
    size_ = size_tmp;
  }

  auto& operator=(const List& oth) {
    if (&oth != this)
    {
      while (size()) {
        pop_back();
      }
      if (std::allocator_traits<NodeAllocator>::propagate_on_container_copy_assignment::value)
      {
        allocatorNode = oth.get_allocator();
      }
      try {
        for (auto& w : oth) {
          push_back(w);
        }
      } catch (...) {
        this->~List();
      }
    }
    return *this;
  }
  auto& operator=(List&& oth) {
    if (this != &oth) {
      this->~List();
      new (this) List(std::move(oth));
    }
    return *this;
  }

  ~List() noexcept {
    while (size()) {
      pop_back();
    }
  }
 private:

  //using NodeAllocator = typename std::allocator_traits<Allocator>::template Rebind_alloc<Node>;
  using NodeAllocator = typename std::allocator_traits<Allocator>::template Rebind_alloc<Node>;

  size_t size_ = 0;

  BaseNode FakeNode;

  //Allocator allocator_;

  NodeAllocator allocatorNode;
  //Allocator allocatorT;

};

