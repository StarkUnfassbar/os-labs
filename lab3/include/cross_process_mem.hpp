#pragma once

#include <string>
#include <stdexcept>

#ifdef _WIN32
#   include <windows.h>
#	define MAP_NAME_PREFIX "Local\\"
#	define INV_HANDLE (NULL)
#	define CSEM        HANDLE
#else
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <semaphore.h>
#   define HANDLE          int
#   define INV_HANDLE      (-1)
#	define MAP_NAME_PREFIX  "/"
#	define CSEM            sem_t*
#endif

namespace cpmem {
    template <class T> class SharedMem {
        private:
            struct Header {
                T data;
                int ref_count;
            };

            std::string mem_name_;
            std::string sem_name_;
            
            #ifdef _WIN32
                HANDLE fd_ = INV_HANDLE;
                HANDLE sem_ = INV_HANDLE;
            #else
                int fd_ = INV_HANDLE;
                sem_t* sem_ = SEM_FAILED;
            #endif
            
            Header* mapped_mem_ = nullptr;

            std::string BuildName(const char* base, const char* suffix = "") {
                return MAP_NAME_PREFIX + std::string(base) + suffix;
            }

        public:
            SharedMem(const char* name, bool create_if_not_exists = true) 
                : mem_name_(BuildName(name)), 
                  sem_name_(BuildName(name, "_sem")) 
            {
                if (!Open() && create_if_not_exists) {
                    Create();
                }
                
                if (!IsValid()) {
                    throw std::runtime_error("Не удалось создать или открыть память");
                }
                
                Map();
                
                Lock();
                mapped_mem_->ref_count++;
                Unlock();
            }
            
            ~SharedMem() {
                if (mapped_mem_) {
                    Lock();
                    bool last_user = (--mapped_mem_->ref_count == 0);
                    Unlock();
                    
                    Unmap();
                    Close();
                    
                    if (last_user) {
                        #ifndef _WIN32
                            shm_unlink(mem_name_.c_str());
                            sem_unlink(sem_name_.c_str());
                        #endif
                    }
                }
            }

            void Close() {
                #ifdef _WIN32
                    if (fd_ != INV_HANDLE) CloseHandle(fd_);
                    if (sem_ != INV_HANDLE) CloseHandle(sem_);
                #else
                    if (fd_ != INV_HANDLE) close(fd_);
                    if (sem_ != SEM_FAILED) sem_close(sem_);
                #endif
                
                fd_ = INV_HANDLE;
                #ifdef _WIN32
                sem_ = INV_HANDLE;
                #else
                sem_ = SEM_FAILED;
                #endif
            }
            
            bool IsValid() const {
                #ifdef _WIN32
                    return fd_ != INV_HANDLE && sem_ != INV_HANDLE && mapped_mem_;
                #else
                    return fd_ != INV_HANDLE && sem_ != SEM_FAILED && mapped_mem_;
                #endif
            }
            
            T* Data() { 
                return IsValid() ? &mapped_mem_->data : nullptr; 
            }
            
            void Lock() {
                #ifdef _WIN32
                    WaitForSingleObject(sem_, INFINITE);
                #else
                    sem_wait(sem_);
                #endif
            }
            
            void Unlock() {
                #ifdef _WIN32
                    ReleaseSemaphore(sem_, 1, NULL);
                #else
                    sem_post(sem_);
                #endif
            }

        private:
            bool Open() {
                #ifdef _WIN32
                    fd_ = OpenFileMapping(FILE_MAP_WRITE, FALSE, mem_name_.c_str());
                    if (fd_ != INV_HANDLE) {
                        sem_ = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, sem_name_.c_str());
                    }
                #else
                    fd_ = shm_open(mem_name_.c_str(), O_RDWR, 0644);
                    if (fd_ != INV_HANDLE) {
                        sem_ = sem_open(sem_name_.c_str(), 0);
                    }
                #endif
                    return IsValid();
            }
            
            bool Create() {
                #ifdef _WIN32
                    fd_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 
                                            PAGE_READWRITE, 0, sizeof(Header), mem_name_.c_str());
                    if (fd_ != INV_HANDLE) {
                        sem_ = CreateSemaphore(NULL, 1, 1, sem_name_.c_str());
                    }
                #else
                    fd_ = shm_open(mem_name_.c_str(), O_CREAT | O_EXCL | O_RDWR, 0644);
                    if (fd_ != INV_HANDLE) {
                        ftruncate(fd_, sizeof(Header));
                        sem_ = sem_open(sem_name_.c_str(), O_CREAT | O_EXCL, 0644, 1);
                    }
                #endif
                    if (IsValid()) {
                        Map();
                        if (mapped_mem_) {
                            mapped_mem_->ref_count = 0;
                            mapped_mem_->data = T();
                        }
                    }
                
                return IsValid();
            }
            
            void Map() {
                #ifdef _WIN32
                    mapped_mem_ = reinterpret_cast<Header*>(
                        MapViewOfFile(fd_, FILE_MAP_WRITE, 0, 0, sizeof(Header)));
                #else
                    void* ptr = mmap(NULL, sizeof(Header), PROT_READ | PROT_WRITE, 
                                    MAP_SHARED, fd_, 0);
                    mapped_mem_ = (ptr != MAP_FAILED) ? reinterpret_cast<Header*>(ptr) : nullptr;
                #endif
            }
            
            void Unmap() {
                if (!mapped_mem_) return;
                
                #ifdef _WIN32
                    UnmapViewOfFile(mapped_mem_);
                #else
                    munmap(mapped_mem_, sizeof(Header));
                #endif
                    mapped_mem_ = nullptr;
            }
            
            void Close() {
                Unmap();
                
                #ifdef _WIN32
                    if (fd_ != INV_HANDLE) CloseHandle(fd_);
                    if (sem_ != INV_HANDLE) CloseHandle(sem_);
                #else
                    if (fd_ != INV_HANDLE) close(fd_);
                    if (sem_ != SEM_FAILED) sem_close(sem_);
                #endif
                
                fd_ = INV_HANDLE;
                #ifdef _WIN32
                sem_ = INV_HANDLE;
                #else
                sem_ = SEM_FAILED;
                #endif
            }
            
            void Destroy() {
                Close();
                
                #ifndef _WIN32
                    shm_unlink(mem_name_.c_str());
                    sem_unlink(sem_name_.c_str());
                #endif
            }
    };
};