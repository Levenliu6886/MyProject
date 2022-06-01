#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTRARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/uio.h>
#include <vector>

namespace sylar {

//二进制数组,提供基础类型的序列化,反序列化功能
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    struct Node {
        Node(size_t s);//内存块字节数
        Node();
        ~Node();

        char* ptr;//内存块地址指针
        Node* next;
        size_t size;
    };
    //使用指定长度的内存块构造ByteArray 字节为单位
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //write 1字节、2字节、4字节、8字节
    //固定长度
    void writeFint8(int8_t value);//写入固定长度int8_t类型的数据
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    //可变长度
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);
    //length:int16 , data
    void writeStringF16(const std::string& value);
    //length:int32 , data
    void writeStringF32(const std::string& value);
    //length:int64 , data
    void writeStringF64(const std::string& value);
    //length:varint , data
    void writeStringVint(const std::string& value);//压缩的//写入std::string类型的数据,用无符号Varint64作为长度类型
    //data
    void writeStringWithoutLength(const std::string& value);

    //read
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    //length:int16, data
    std::string readStringF16();
    //length:int32, data
    std::string readStringF32();
    //length:int64, data
    std::string readStringF64();
    //length:varint, data
    std::string readStringVint();

    //内部操作
    void clear();

    void write(const void* buf, size_t size);//往buf里面写一个长度
    void read(void* buf, size_t size);//读取size长度的数据
    void read(void* buf, size_t size, size_t position) const;//从position开始读取size长度的数据
    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);

    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    size_t getBaseSize() const { return m_baseSize; }
    size_t getReadSize() const { return m_size - m_position; }//返回可读取数据大小

    bool isLittleEndian() const;
    void setLittleEndian(bool val);

    std::string toString() const;//将ByteArray里面的数据[m_position, m_size)转成std::string
    std::string toHexString() const;//将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)


    //只获取内容不修改position
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;//获取可读取的缓存,保存成iovec数组
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    //增加容量，不改position
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    size_t getSize() const { return m_size; }
private:
    void addCapacity(size_t size);//扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
    size_t getCapacity() const { return m_capacity - m_position; }//获取当前的可写入容量
private:
    size_t m_baseSize;//内存块的大小
    size_t m_position;//当前操作位置
    size_t m_capacity;//当前的总容量
    size_t m_size;    //当前数据的大小
    int8_t m_endian;  //字节序,默认大端

    Node* m_root;     //第一个内存块指针
    Node* m_cur;      //当前内存块指针
};

}

#endif