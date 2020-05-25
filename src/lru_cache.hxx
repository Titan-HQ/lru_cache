/***************************************************************************
 *   Copyright (C) 2004-2011 by Patrick Audley                             *
 *   paudley@blackcat.ca                                                   *
 *   http://patrickaudley.com                                              *
 *                                                                         *
 ***************************************************************************/
#include <unordered_map>
#include <list>
#include <vector>

#ifndef LRU_CACHE_HXX
#define LRU_CACHE_HXX


template < class T >
struct Countfn {
      constexpr
      unsigned long operator()( const T & ) const noexcept { return 1; }
};


/**
 * @brief Template cache with an LRU removal policy.
 * @class LRUCache
 *
 * @par
 * This template creats a simple collection of key-value pairs that grows
 * until the size specified at construction is reached and then begins
 * discard the Least Recently Used element on each insertion.
 *
 */
template< class Key, class Data, class Sizefn = Countfn< Data > > 
class LRUCache {
   public:
      typedef std::list< std::pair< Key, Data > > List;         ///< Main cache storage typedef
      typedef typename List::iterator List_Iter;                ///< Main cache iterator
      typedef typename List::const_iterator List_cIter;         ///< Main cache iterator (const)
      typedef std::vector< Key > Key_List;                      ///< List of keys
      typedef typename Key_List::iterator Key_List_Iter;        ///< Main cache iterator
      typedef typename Key_List::const_iterator Key_List_cIter; ///< Main cache iterator (const)
      typedef std::unordered_map< Key, List_Iter > Map;                   ///< Index typedef
      typedef std::pair< Key, List_Iter > Pair;                 ///< Pair of Map elements
      typedef typename Map::iterator Map_Iter;       ///< Index iterator
      typedef typename Map::const_iterator Map_cIter;           ///< Index iterator (const)

   private:
      List  list_;               ///< Main cache storage
      Map   index_;               ///< Cache storage index
      unsigned long max_size_;  ///< Maximum abstract size of the cache
      unsigned long curr_size_; ///< Current abstract size of the cache

   public:

      /*
       * Create a cache that holds at most Size worth of elements
       */
      LRUCache( const unsigned long Size ) :max_size_( Size ),curr_size_( 0 ){}

      /*
       * Destructor - cleans up both index and storage
       */
      virtual ~LRUCache() { 
         clear(); 
      }

      /* 
       * Get the current size of the cache.
       */
      inline unsigned long size( void ) const { return curr_size_; }

      /*
       * Get the maximum sbstract size of the cache.
       */
      inline  unsigned long max_size( void ) const { return max_size_; }

      /*
       * Clear all storage and indices.
       */
      void clear( void ) {
         list_.clear();
         index_.clear();
         curr_size_=0;
      }

      /*
       * Check for the existance of a key in the cache.
       */
      inline bool exists(const Key &key) const {
         return index_.find(key) != index_.end();
      }

      /*
       * Remove a key-data pair from the cache.
       */
      inline void remove( const Key &key ) {
         Map_Iter miter = index_.find(key);
         if (miter == index_.end())
            return;
         remove_( miter );
      }

      /*
       * Touches a key in the Cache and makes it the most recently used.
       */
      inline void touch( const Key &key ) {
         touch_( key );
      }

      /*
       * Fetches a pointer to cache data.
       *  @param key to fetch data for
       *  @param touch whether or not to touch the data
       *  @return pointer to data or NULL on error
       */
      inline Data *fetch_ptr( const Key &key ) {

         Map_Iter miter = index_.find( key );
         if( miter == index_.end() )
            return NULL;
         touch_( key );
         return &(miter->second->second);
      }

      /*
       * Fetches a copy of cached data.
       *  @param key to fetch data for
       *  @param touch_data whether or not to touch the data
       *  @return copy of the data or an empty Data object if not found
       */
      inline Data fetch(const Key &key, bool touch_data = true) {
         Map_Iter miter = index_.find( key );
         if(miter == index_.end())
            return Data();
         Data tmp = miter->second->second;
         if(touch_data)
            touch_( key );
         return tmp;
      }

      /*
       * Fetches a pointer to cache data.
       *  @param key to fetch data for
       *  @param data to fetch data into
       *  @param touch_data whether or not to touch the data
       *  @return whether or not data was filled in
       */
      inline bool fetch( const Key &key, Data &data, bool touch_data = true ) {
         Map_Iter miter = index_.find( key );
         if(miter == index_.end())
            return false;
         if(touch_data)
            touch_( key );
         data = miter->second->second;
         return true;
      }

      /*
       * Inserts a key-data pair into the cache and removes entries if neccessary.
       *  @param key object key for insertion
       *  @param data object data for insertion
       *  @note This function checks key existance and touches the key if it already exists.
       */
      inline void insert( const Key &key, const Data &data ) {
         // Touch the key, if it exists, then replace the content.
         Map_Iter miter = touch_( key );
         if(miter != index_.end())
            remove_( miter );

         // Ok, do the actual insert at the head of the list
         list_.push_front(std::make_pair(key, data));
         List_Iter liter = list_.begin();

         // Store the index
         index_.insert( std::make_pair(key, liter));
         curr_size_ += Sizefn()( data );

         // Check to see if we need to remove an element due to exceeding max_size
         while(curr_size_ > max_size_) {
            // Remove the last element.
            liter = list_.end();
            --liter;
            remove_( liter->first );
         }
      }

      /*
       * Get a list of keys.
       *  @return list of the current keys.
       */
      inline  Key_List get_all_keys( void ) {
         Key_List ret{};
         for (auto & liter : list_) ret.push_back( liter->first );
         return ret;
      }

      bool get_key_for_data( const Data & a_data, Key & a_key )
      {
          for ( auto & l_i : list_ ) {
              if ( l_i.second == a_data ) {
                  a_key = l_i.first;
                  return true;
              }
          }
          return false;
      }
      
   private:
      /*
       * Internal touch function.
       *  @param key to be touched
       *  @return a Map_Iter pointing to the key that was touched.
       */
      inline Map_Iter touch_( const Key &key ) {
         Map_Iter miter = index_.find(key);
         if (miter == index_.end())
            return miter;
         // Move the found node to the head of the list.
         list_.splice(list_.begin(),list_, miter->second);
         return miter;
      }

      /* 
       * Interal remove function
       *  @param miter Map_Iter that points to the key to remove
       *  @warning miter is now longer usable after being passed to this function.
       */
      inline void remove_( const Map_Iter &miter ) {
         curr_size_ -= Sizefn()( miter->second->second );
         list_.erase(miter->second);
         index_.erase(miter);
      }

      /*
       * Interal remove function
       *  @param key to remove
       */
      inline void remove_( const Key &key ) {
         Map_Iter miter = index_.find( key );
         remove_(miter);
      }
};

// END DEFINITION LRU_CACHE_HXX
#endif
