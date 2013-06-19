// Copyright (c) 2012, Cloudera, inc.

#ifndef KUDU_CFILE_INDEX_BTREE_H
#define KUDU_CFILE_INDEX_BTREE_H

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>
#include <memory>

#include "cfile.pb.h"
#include "cfile/block_cache.h"
#include "index_block.h"
#include "util/logging.h"

namespace kudu {
namespace cfile {

using boost::ptr_vector;

class CFileReader;
class Writer;

class IndexTreeBuilder {
public:
  explicit IndexTreeBuilder(
    const WriterOptions *options,
    Writer *writer);

  // Append the given key into the index.
  // The key is copied into the builder's internal
  // memory.
  Status Append(const Slice &key, const BlockPointer &block);
  Status Finish(BTreeInfoPB *info);
private:
  IndexBlockBuilder *CreateBlockBuilder(bool is_leaf);
  Status Append(const Slice &key, const BlockPointer &block_ptr,
                size_t level);

  // Finish the current block at the given index level, and then
  // propagate by inserting this block into the next higher-up
  // level index.
  Status FinishBlockAndPropagate(size_t level);

  // Finish the current block at the given level, writing it
  // to the file. Return the location of the written block
  // in 'written'.
  Status FinishAndWriteBlock(size_t level, BlockPointer *written);

  const WriterOptions *options_;
  Writer *writer_;

  ptr_vector<IndexBlockBuilder> idx_blocks_;
};

class IndexTreeIterator : boost::noncopyable {
public:
  explicit IndexTreeIterator(
      const CFileReader *reader,
      const BlockPointer &root_blockptr);

  Status SeekToFirst();
  Status SeekAtOrBefore(const Slice &search_key);
  bool HasNext();
  Status Next();

  // The slice key at which the iterator
  // is currently seeked to.
  const Slice GetCurrentKey() const;
  const BlockPointer &GetCurrentBlockPointer() const;

  static IndexTreeIterator *Create(
    const CFileReader *reader,
    DataType type,
    const BlockPointer &idx_root);

private:
  IndexBlockIterator *BottomIter();
  IndexBlockReader *BottomReader();
  IndexBlockIterator *seeked_iter(int depth);
  IndexBlockReader *seeked_reader(int depth);
  Status LoadBlock(const BlockPointer &block, int dept);
  Status SeekDownward(const Slice &search_key, const BlockPointer &in_block,
                      int cur_depth);
  Status SeekToFirstDownward(const BlockPointer &in_block, int cur_depth);

  struct SeekedIndex {
    SeekedIndex() :
      iter(&reader)
    {}

    // Hold a copy of the underlying block data, which would
    // otherwise go out of scope. The reader and iter
    // do not themselves retain the data.
    BlockPointer block_ptr;
    BlockCacheHandle data;
    IndexBlockReader reader;
    IndexBlockIterator iter;
  };

  const CFileReader *reader_;

  BlockPointer root_block_;

  ptr_vector<SeekedIndex> seeked_indexes_;

};

} // namespace cfile
} // namespace kudu
#endif
