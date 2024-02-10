/*
 *    ________  ________  .____      __________.__
 *    \______ \ \______ \ |    |     \______   \  |  __ __   ____
 *     |    |  \ |    |  \|    |      |    |  _/  | |  |  \_/ __ \
 *     |    `   \|    `   \    |___   |    |   \  |_|  |  /\  ___/
 *    /_______  /_______  /_______ \  |______  /____/____/  \___  >
 *            \/        \/        \/         \/                 \/
 */

#pragma once

#include <cstdint>
#include <memory>

using size_t = std::size_t;

namespace BlueBinXml::Convertor{
    class CConvertor; 
}
namespace BlueBinXml{
    
    using TOffset = uint32_t;
    using TCount = uint32_t;

    

    /*======================================================================*/
    // TODO alignas
    class CNode{
    private:
        struct SAttributeDef{
            TOffset m_Name;
            TOffset m_Value;
        }; 

        /*----------------------------------------------------------------------*/
        template<typename T>
        const T* GetObject(TOffset offset) const{
            return reinterpret_cast<const T*> (reinterpret_cast<const uint8_t*>(this)+offset);
        }

        /*----------------------------------------------------------------------*/
        template<typename T>
        T* GetObject(TOffset offset){
            return reinterpret_cast<T*> (reinterpret_cast<uint8_t*>(this)+offset);
        }

        /*----------------------------------------------------------------------*/
        static TOffset GetAttributeDefOffset(TCount id){
            return sizeof(CNode) + id*sizeof(SAttributeDef);
        }

        /*----------------------------------------------------------------------*/
        TOffset GetChildRefOffset(TCount id) const{
            return m_AttributeCount*sizeof(SAttributeDef) + id*sizeof(TOffset);
        }

    public:
        friend class Convertor::CConvertor;

        struct SAttribute{
            const char * m_Name;
            const char * m_Value;
        };

        /*----------------------------------------------------------------------*/
        const char* GetName() const{
            return GetObject<char>(m_Name);
        }

        /*----------------------------------------------------------------------*/
        const char* GetContent() const{
            return GetObject<char>(m_Content);
        }

        /*----------------------------------------------------------------------*/
        TCount GetAttributeCount() const{
            return m_AttributeCount;
        }

        /*----------------------------------------------------------------------*/
        SAttribute GetAttribute(TCount id) const{
            if (id >= m_AttributeCount){
                return {nullptr, nullptr};
            }
            
            const SAttributeDef& attributeDef = *GetObject<SAttributeDef>(GetAttributeDefOffset(id));
            SAttribute result = {
                GetObject<char>(attributeDef.m_Name),
                GetObject<char>(attributeDef.m_Value)
            };

            return result;
        }

        /*----------------------------------------------------------------------*/
        TCount GetChildernCound() const{
            return m_ChildCount;
        }

        /*----------------------------------------------------------------------*/
        const CNode* GetChild(TCount id) const{
            if (id >= m_ChildCount){
                return nullptr;
            }

            TOffset nodeOffset = *GetObject<TOffset>(GetChildRefOffset(id));
            return GetObject<CNode>(nodeOffset);
        }

    private:
        CNode() = default;
        TOffset m_Name;
        TOffset m_Content;
        TCount  m_AttributeCount;
        TCount  m_ChildCount;

    };

    /*======================================================================*/
    class CDocument{
    public:
        CDocument(std::unique_ptr<void*> data, const size_t size)
        : m_Data(std::move(data))
        , m_DataSize(size)
        , m_IsValid(size>sizeof(CNode)){ 
            // TODO proper check, check data for null
        }
        
        const CNode* GetRootNode() const{
            if (!m_IsValid){
                return nullptr;
            }
            return reinterpret_cast<CNode*>(m_Data.get());
        }


    private:
        std::unique_ptr<void*>  m_Data;
        size_t                  m_DataSize;
        bool                    m_IsValid;
    };
}


