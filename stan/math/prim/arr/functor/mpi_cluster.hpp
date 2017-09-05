#pragma once

#include <mutex>

#include <boost/mpi.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <stan/math/prim/arr/functor/mpi_command.hpp>

namespace stan {
  namespace math {

    // MPI command which will shut down a child gracefully
    struct mpi_stop_worker : public mpi_command {
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive & ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(mpi_command);
      }
      void run() const {
        boost::mpi::communicator world;
        std::cout << "Terminating worker " << world.rank() << std::endl;
        MPI_Finalize();
        std::exit(0);
      }
    };

    template<typename T>
    struct mpi_distributed_apply : public mpi_command {
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive & ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(mpi_command);
      }
      void run() const {
        T::distributed_apply();
      }
    };

    template<typename T>
    void mpi_broadcast_command() {
      boost::mpi::communicator world;

      if(world.rank() != 0)
        throw std::runtime_error("only root may broadcast commands.");

      // used to lock the mpi cluster during execution of some
      // distributed task
      static std::mutex mpi_cluster_mutex;
      std::lock_guard<std::mutex> lock_cluster(mpi_cluster_mutex);
      
      boost::shared_ptr<mpi_command> command(new T);
      
      boost::mpi::broadcast(world, command, 0);
    }

    // map N chunks to W with a chunks-size of C; used for
    // deterministic scheduling
    std::vector<int>
    mpi_map_chunks(std::size_t N, std::size_t C = 1) {
      boost::mpi::communicator world;
      const std::size_t W = world.size();
        
      std::vector<int> chunks(W, N / W);
      
      for(std::size_t r = 0; r != N % W; r++)
        ++chunks[r+1];

      for(std::size_t i = 0; i != W; i++)
        chunks[i] *= C;

      return(chunks);
    }

    struct mpi_cluster {
      boost::mpi::communicator world_;
      std::size_t const R_ = world_.rank();
      
      mpi_cluster() {
        if(R_ != 0) {
          std::cout << "Worker " << R_ << " waiting for commands..." << std::endl;
          while(1) {
            boost::shared_ptr<mpi_command> work;
            
            boost::mpi::broadcast(world_, work, 0);

            work->run();
          }
        }
      }

      ~mpi_cluster() {
        // the destructor will ensure that the childs are being
        // shutdown
        if(R_ == 0) {
          mpi_broadcast_command<mpi_stop_worker>();
        }
      }


    };

  }
}

BOOST_CLASS_EXPORT(stan::math::mpi_stop_worker)
BOOST_CLASS_TRACKING(stan::math::mpi_stop_worker,track_never)
BOOST_SERIALIZATION_FACTORY_0(stan::math::mpi_stop_worker)
