#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <type_traits>
#include <atomic>
#include <stdexcept>
#include <algorithm>

namespace threading {

class ThreadPool {
public:
    explicit ThreadPool( size_t num_threads = 0 ) : m_stop{ false } {
        if( num_threads == 0 ) {
            unsigned int hardware = std::thread::hardware_concurrency( );
            num_threads = hardware > 0 ? hardware : 4;
        }

        m_workers.reserve( num_threads );
        for( size_t i = 0; i < num_threads; ++i ) {
            m_workers.emplace_back( [ this ] {
                while( true ) {
                    std::function< void( ) > task;
                    {
                        std::unique_lock< std::mutex > lock( this->m_queue_mutex );
                        this->m_cv.wait( lock, [ this ] {
                            return this->m_stop.load( ) || !this->m_tasks.empty( );
                        } );

                        if( this->m_stop.load( ) && this->m_tasks.empty( ) )
                            return;

                        task = std::move( this->m_tasks.front( ) );
                        this->m_tasks.pop( );
                    }

                    task( );
                }
            } );
        }
    }

    ~ThreadPool( ) {
        shutdown( );
    }

    ThreadPool( const ThreadPool& ) = delete;
    ThreadPool& operator=( const ThreadPool& ) = delete;
    ThreadPool( ThreadPool&& ) = delete;
    ThreadPool& operator=( ThreadPool&& ) = delete;

    void shutdown( ) {
        if( !m_stop.exchange( true ) ) {
            m_cv.notify_all( );
            for( std::thread& worker : m_workers ) {
                if( worker.joinable( ) )
                    worker.join( );
            }
            m_workers.clear( );
        }
    }

    template< typename F, typename... Args >
    auto enqueue( F&& f, Args&&... args ) 
        -> std::future< typename std::invoke_result< F, Args... >::type > 
    {
        using return_type = typename std::invoke_result< F, Args... >::type;

        auto task = std::make_shared< std::packaged_task< return_type( ) > >(
            std::bind( std::forward< F >( f ), std::forward< Args >( args )... )
        );

        std::future< return_type > res = task->get_future( );
        {
            std::unique_lock< std::mutex > lock( m_queue_mutex );
            if( m_stop.load( ) )
                throw std::runtime_error( "enqueue on stopped ThreadPool" );

            m_tasks.emplace( [ task ]( ) { ( *task )( ); } );
        }
        m_cv.notify_one( );
        return res;
    }

    template< typename Index, typename Func >
    void parallel_for( Index begin, Index end, Func&& func ) {
        if( begin >= end )
            return;

        const size_t total = static_cast< size_t >( end - begin );
        const size_t num_threads = m_workers.empty( ) ? 1 : m_workers.size( );
        const size_t chunk_size = std::max( size_t{ 1 }, ( total + num_threads - 1 ) / num_threads );

        std::vector< std::future< void > > futures;
        futures.reserve( num_threads );

        for( Index i = begin; i < end; i += static_cast< Index >( chunk_size ) ) {
            Index chunk_end = std::min( end, static_cast< Index >( i + chunk_size ) );
            futures.push_back( enqueue( [ i, chunk_end, &func ] {
                for( Index idx = i; idx < chunk_end; ++idx ) {
                    func( idx );
                }
            } ) );
        }

        for( auto& f : futures ) {
            f.get( );
        }
    }

    [[nodiscard]] size_t size( ) const noexcept {
        return m_workers.size( );
    }

    [[nodiscard]] bool is_running( ) const noexcept {
        return !m_stop.load( );
    }

private:
    std::vector< std::thread >             m_workers;
    std::queue< std::function< void( ) > > m_tasks;
    std::mutex                             m_queue_mutex;
    std::condition_variable                m_cv;
    std::atomic< bool >                    m_stop;
};

inline ThreadPool& get_pool( ) {
    static ThreadPool pool;
    return pool;
}

} // namespace threading
