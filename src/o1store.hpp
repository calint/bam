#pragma once
//
// implements a O(1) store of objects
//
// * Type is object type. 'Type' must contain public field 'Type **alloc_ptr'
// * Size is number of pre-allocated objects
// * StoreId is for debugging
// * InstanceSizeInBytes is custom size of instance
//   used to fit largest object in an object hierarchy
//
// note. no destructor since life-time is program life-time
//
template <typename Type, const unsigned Size, const unsigned StoreId = 0,
          const unsigned InstanceSizeInBytes = 0>
class o1store {
  Type *all_ = nullptr;
  Type **free_bgn_ = nullptr;
  Type **free_ptr_ = nullptr;
  Type **free_end_ = nullptr;
  Type **alloc_bgn_ = nullptr;
  Type **alloc_ptr_ = nullptr;
  Type **del_bgn_ = nullptr;
  Type **del_ptr_ = nullptr;
  Type **del_end_ = nullptr;

public:
  o1store() {
    if (InstanceSizeInBytes) {
      all_ = (Type *)calloc(Size, InstanceSizeInBytes);
    } else {
      all_ = (Type *)calloc(Size, sizeof(Type));
    }
    free_ptr_ = free_bgn_ = (Type **)calloc(Size, sizeof(Type *));
    free_end_ = free_bgn_ + Size;
    alloc_ptr_ = alloc_bgn_ = (Type **)calloc(Size, sizeof(Type *));
    del_ptr_ = del_bgn_ = (Type **)calloc(Size, sizeof(Type *));
    del_end_ = del_bgn_ + Size;
    if (!all_ or !free_bgn_ or !alloc_bgn_ or !del_bgn_) {
      Serial.printf("!!! o1store %u: could not allocate arrays\n", StoreId);
      while (true)
        ;
    }
    // write pointers to instances in the 'free' list
    Type *all_it = all_;
    for (Type **free_it = free_bgn_; free_it < free_end_; free_it++) {
      *free_it = all_it;
      if (InstanceSizeInBytes) {
        all_it = (Type *)((char *)all_it + InstanceSizeInBytes);
      } else {
        all_it++;
      }
    }
  }

  // virtual ~o1store() {
  //   free(all_);
  //   free(free_);
  //   free(alloc_);
  //   free(del_);
  // }

  // returns true if allocatable instances available
  inline auto can_allocate() -> bool { return free_ptr_ < free_end_; }

  // allocates an instance
  auto allocate_instance() -> Type * {
    if (free_ptr_ >= free_end_) {
      return nullptr;
    }
    Type *inst = *free_ptr_;
    free_ptr_++;
    *alloc_ptr_ = inst;
    inst->alloc_ptr = alloc_ptr_;
    alloc_ptr_++;
    return inst;
  }

  // adds instance to a list that is applied with 'apply_free()'
  void free_instance(Type *inst) {
    if (del_ptr_ >= del_end_) {
      Serial.printf("!!! o1store %u: free overrun\n", StoreId);
      while (true)
        ;
    }
    *del_ptr_ = inst;
    del_ptr_++;
  }

  // de-allocates the instances that have been freed
  void apply_free() {
    for (Type **it = del_bgn_; it < del_ptr_; it++) {
      Type *inst_deleted = *it;
      alloc_ptr_--;
      Type *inst_to_move = *alloc_ptr_;
      inst_to_move->alloc_ptr = inst_deleted->alloc_ptr;
      *(inst_deleted->alloc_ptr) = inst_to_move;
      free_ptr_--;
      *free_ptr_ = inst_deleted;
    }
    del_ptr_ = del_bgn_;
  }

  // returns pointer to list of allocated instances
  inline auto allocated_list() -> Type ** { return alloc_bgn_; }

  // returns size of list of allocated instances
  inline auto allocated_list_len() -> unsigned {
    return alloc_ptr_ - alloc_bgn_;
  }

  // returns the list with all pre-allocated instances
  inline auto all_list() -> Type * { return all_; }

  // returns the size of 'all' list
  constexpr auto all_list_len() -> unsigned { return Size; }

  // returns instance from 'all' list at index 'ix'
  inline auto instance(unsigned ix) -> Type * {
    if (!InstanceSizeInBytes) {
      return &all_[ix];
    }
    // note. if instance size is specified do pointer shenanigans
    return (Type *)((char *)all_ + InstanceSizeInBytes * ix);
  }

  // returns the size in bytes of allocated heap memory
  constexpr auto allocated_data_size_B() -> size_t {
    if (InstanceSizeInBytes) {
      return Size * InstanceSizeInBytes + 3 * Size * sizeof(Type *);
    }
    return Size * sizeof(Type) + 3 * Size * sizeof(Type *);
  }
};