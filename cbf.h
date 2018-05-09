#ifndef COUNTING_CRUEL_BLOOM__
#define COUNTING_CRUEL_BLOOM__
#include "bf.h"
#include "aesctr.h"

namespace bf {

template<typename HashStruct=WangHash, typename RngType=aes::AesCtr<std::uint64_t, 8>>
class cbfbase_t {
protected:
    std::vector<bfbase_t<HashStruct>> bfs_;
    RngType   rng_;
    uint64_t  gen_;
    uint8_t nbits_;
    // TODO: this can be improved by providing a continuous chunk of memory
    //       and performing all operations on subfilters.
public:
    explicit cbfbase_t(size_t nbfs, size_t l2sz, unsigned nhashes, uint64_t seedseedseedval): rng_{seedseedseedval}, gen_(rng_()), nbits_(64) {
        if(!nbfs) throw std::runtime_error("Need at least 1.");
        bfs_.reserve(nbfs);
        while(bfs_.size() < nbfs) bfs_.emplace_back(l2sz, nhashes, rng_());
    }
    INLINE void addh(uint64_t val) {
        auto it(bfs_.begin());
        if(!it->may_contain(val)) {
            // std::fprintf(stderr, "Item %" PRIu64 " not contained in first filter. Adding\n", val);
            it->addh(val);
            return;
        }
        for(++it;it < bfs_.end() && it->may_contain(val);++it);
        if(it == bfs_.end()) {
            // std::fprintf(stderr, "The structure is filled.\n");
            return;
        }
        assert(it->may_contain(val) == 0);
        // Otherwise, insert at position.
        const auto dist = static_cast<unsigned>(std::distance(bfs_.begin(), it));
        // std::fprintf(stderr, "Number of slots ahead of 0: %u\n", dist);
        if(__builtin_expect(nbits_ < dist, 0)) gen_ = rng_(), nbits_ = 64;
        if((gen_ & (UINT64_C(-1) >> (64 - dist))) == 0) {
            // std::fprintf(stderr, "Seeing if all random bits are equal to bitmask %" PRIx64 "\n", (UINT64_C(-1) >> (64 - dist)));
            it->addh(val);
        }
        gen_ >>= dist;
        nbits_ -= dist;
    }
    bool may_contain(uint64_t val) const {
        return bfs_[0].may_contain(val);
    }
    unsigned est_count(uint64_t val) const {
        auto it(bfs_.cbegin());
        if(!it->may_contain(val)) {
            std::fprintf(stderr, "%" PRIu64 " is not contained\n", val);
            return 0;
        }
        while(it->may_contain(val) && it < bfs_.end()) ++it;
        return 1u << (std::distance(bfs_.cbegin(), it) - 1);
    }
    void resize_sketches(unsigned np) {
        for(auto &bf: bfs_) bf.resize(np);
    }
    void resize(unsigned nbfs) {
        bfs_.reserve(nbfs);
        auto nhashes = bfs_.at(0).nhashes();
        auto np = bfs_[0].p();
        for(auto &bf: bfs_) bf.clear();
        while(bfs_.size() < nbfs) bfs_.emplace_back(np, nhashes, rng_());
    }
    void clear() {
        for(auto &bf: bfs_) bf.clear();
    }
    auto p() const {
        return bfs_[0].p();
    }
    auto nhashes() const {
        return bfs_[0].nhashes();
    }
    std::size_t size() const {return bfs_.size();}
    std::size_t filter_size() const {return bfs_[0].size();}
    auto begin() const {return bfs_.cbegin();}
    auto end() const {return bfs_.cend();}
};
using cbf_t = cbfbase_t<>;

} // namespace bf

#endif // #ifndef COUNTING_CRUEL_BLOOM__
