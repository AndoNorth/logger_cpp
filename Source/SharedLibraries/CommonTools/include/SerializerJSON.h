#pragma once

#include <algorithm>
#include <ranges>
#include <type_traits>

#include <concepts>
#include <iostream>
#include <exception>

#ifdef _WINDOWS
#include <atlstr.h>
#else
#include <WinEmul.h>
#endif

// Inhibit annoying VS headers bleeding their own "min" and "max" macros allover the place, and 
// messes up calls to std::min and std::max. An alternatve is to write "(std::min)(...)" instead of "std::min(...)"
#pragma push_macro("max")
#pragma push_macro("min")
#undef max
#undef min

#ifdef _WINDOWS
#ifdef COMMONTOOLS_STATIC
   #define COMMONTOOLS_EXPORT
   #define MESSIRTOOLS_EXPORT
#else
   #ifdef _COMMONTOOLS
      #define COMMONTOOLS_EXPORT       AFX_CLASS_EXPORT
      #define MESSIRTOOLS_EXPORT       AFX_CLASS_EXPORT
   #else
      #define COMMONTOOLS_EXPORT       AFX_CLASS_IMPORT
      #define MESSIRTOOLS_EXPORT       AFX_CLASS_IMPORT
   #endif
#endif
#else // _WINDOWS
   #define COMMONTOOLS_EXPORT
   #define MESSIRTOOLS_EXPORT
#endif //_WINDOWS

// C++20 serialization helpers

#include <tuple>
#include <functional>
#include <nlohmann/json.hpp>

/**
 * Methods to declare and traverse members of a class.
 */


/**
 * Retrieve offset of a member.
 * 
 * @param member
 * @return offset of the member
 */
template<typename T, typename U> constexpr size_t offset_of(U T::* member) {
   return (char*) &((T*)nullptr->*member) - (char*)nullptr;
}

/**
 * Function type used to transform a data before returning it.
 * see class MenuAccess class for an example
 */
template <typename Instance, typename T>
using get_filter_type = std::function<T&(Instance&, T&)>;

/**
 * Function type used to transform a data before setting it.
 * see class MenuAccess class for an example
 */
template <typename Instance, typename T>
using set_filter_type = std::function<void(Instance&, T&, const T&)>;

/**
 * Template class member type.
 */
template <typename Class, typename T>
using member_ptr_t = T Class::*;

class MemberBase {};

/**
 * Represents a class member variable.
 */
template <typename Class, typename T>
class Member : public MemberBase
{
public:
   const char* name;
   T Class::* ptr;
   size_t offset;
   Class* instance;
   get_filter_type<Class, T> get_filter;
   set_filter_type<Class, T> set_filter;
   std::string description;

public:
   using class_type = Class;
   using member_type = T;

   Member(const char* name, T Class::* ptr, size_t offset, Class* instance,
      get_filter_type<Class, T> get_filter /*= [] (T& value) -> T& { return value; }*/,
      set_filter_type<Class, T> set_filter /*= [] (T& value, const T& new_value) { value = new_value; }*/,
      std::string description = "")
      :
      name(name),
      ptr(ptr),
      offset(offset),
      instance(instance),
      get_filter(get_filter),
      set_filter(set_filter),
      description(description) {}

   T& GetValue() const {
       if (this->instance == nullptr) {
           throw std::runtime_error("no instance");
       } else {
           return this->get_filter(*this->instance, this->instance->*ptr);
       }
   }

   std::string GetName() const {
      return this->name;
   }

   std::string GetDescription() const {
      return this->description;
   }

   void SetValue(const T& value) {
      if (this->instance == nullptr) {
         throw std::runtime_error("no instance");
      } else {
         this->set_filter(*this->instance, this->instance->*ptr, value);
      }
   }
};

/**
 * Returns a tuple composed of members of the parameter Member types.
 * 
 * @param ...args Member types
 * @return a tuple of Member types
 */
template <typename... Args>
auto members(Args&&... args) {
   return std::make_tuple(std::forward<Args>(args)...);
}


/**
 * Bunch of helpers to retrieve a certain tuple item at runtime.
 */

template<
    typename Tuple,
    typename F,
    typename Indices = std::make_index_sequence<std::tuple_size<Tuple>::value>>
    struct runtime_get_func_table;

template<typename Tuple, typename F, size_t I>
void applyForIndex(Tuple& t, F f) {
    f(std::get<I>(t));
}

template<typename Tuple, typename F, size_t ... Indices>
struct runtime_get_func_table<Tuple, F, std::index_sequence<Indices...>>
{
    using FuncType = void(*)(Tuple&, F);
    static constexpr FuncType table[] = {
        &applyForIndex<Tuple, F, Indices>...
    };
};

template<typename Tuple, typename F>
void runtime_get(Tuple& t, size_t index, F f) {
    using tuple_type = typename std::remove_reference<Tuple>::type;
    if (index >= std::tuple_size<tuple_type>::value)
        throw std::runtime_error("Out of range");
    runtime_get_func_table<tuple_type, F>::table[index](t, f);
}



/**
 * unused method to retrieve a particular Member object, use as reference : 
 * https://github.com/eliasdaler/MetaStuff
 */

//template<std::size_t TupleIndex = 0, typename... MemberType>
//auto& get_member_value(std::tuple<MemberType...>& members, const char* name) {
//    
//    if constexpr (TupleIndex < sizeof...(MemberType)) {
//        auto& _member = std::get<TupleIndex>(members);
//
//        if constexpr (_member.GetName() == std::string(name)) {
//            return _member.GetValue();
//        } else {
//            return get_member_value<TupleIndex + 1, MemberType...>(members, name);
//        }
//    } else {
//        throw std::runtime_error(std::string(name) + " : member not found");
//    }
//}


///**
// * Unfinished method to retrieve a particular Member object, at compile time.
// */
//template <typename InstanceType, typename MemberType>
//MemberType& GetMemberValue2(InstanceType& obj, const char* name) {
//   auto members = const_cast<InstanceType&>(obj).GetExposedMembers();
//
//   MemberType* value;
//
//   std::apply(
//      [&] (auto&&... _member) {
//      if constexpr (std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<decltype(_member)>>, Member<MemberType, MemberType> >) {
//            if (_member.GetName() == std::string(name)) {
//               value = (MemberType*) & _member.GetValue();
//            }
//         }
//      },
//      members
//   );
//
//   return *value;
//}

/**
 * Retrieves the value of a particular Member of the given class instance, at runtime.
 * 
 * @param obj instance from which to retrieve the member value
 * @param name the name of the member of which we want to get the value
 * @return reference to the value of the member of the instance
 */
template <typename InstanceType, typename MemberType>
MemberType& GetMemberValue(InstanceType& obj, const char* name) {
   auto members = const_cast<InstanceType&>(obj).GetExposedMembers();

   MemberType* member_value;

   for (int index = 0; index < std::tuple_size<decltype(members)>{}; index++) {
      runtime_get(members, index, [&member_value, name] (auto& member) {
         if (member.GetName() == std::string(name)) {
            member_value = (MemberType*) &member.GetValue();
         }
      });
   }
    
   return *member_value;
}


/**
 * Sets the value of a particular member of the given class instance, at runtime.
 * 
 * @param obj instance of which to set the member value
 * @param name the name of the member of which we want to set the value
 * @param value value we want to set to that object member
 */
template <typename T, typename MemberType>
void SetMemberValue(T& obj, const char* name, MemberType value) {

   auto members = const_cast<T&>(obj).GetExposedMembers();

   for (int index = 0; index < std::tuple_size(members); index++) {
      auto& member = runtime_get(members, index);
      if (member.GetName() == std::string(name)) {
         member.SetValue(value);
         return;
      }
   }

   throw std::runtime_error("member not found");
   return;
}

/**
 * Create a member with default get and set filters. see class MenuAccess class for usage example 
 * of get and set filters.
 * 
 * @param name name given to that member
 * @param ptr address offset of that member, relative to the class instance
 * @param instance class instance from which we are setting a member
 * @param get_filter function called when the member is retrieved
 * @param set_filter function called when the member is assigned a new value
 * @return 
 */
template <typename Class, typename T>
Member<Class, T> member(const char* name, T Class::* ptr, Class* instance,
   std::string description = "",
   //default get filter
   get_filter_type<Class, T> get_filter = [] (Class& instance, T& value) -> T& { return value; },
   //default set filter
   set_filter_type<Class, T> set_filter = [] (Class& instance, T& value, const T& new_value) {
      
      if constexpr (std::is_array_v<std::remove_reference_t<T>>) {
         for (int i = 0; i < sizeof(new_value); i++) {
            value[i] = new_value[i];
         }
      } else if constexpr (!std::is_const_v<std::remove_reference_t<T>>) {
         value = new_value;
      }
   })
{
   return Member<Class, T>(name, ptr, offset_of(ptr), instance, get_filter, set_filter, description);
}


class COMMONTOOLS_EXPORT JSONSerializer;

/**
 * Interface that must be implemented by classes meant for serialization.
 * This also implies implementing a GetExposedMembers method
 */
class COMMONTOOLS_EXPORT JSONSerializable
{
public:
   //virtual void Serialize(std::ostream& serializer) = 0;
   //virtual void Deserialize(std::istream& serializer) = 0;
   
   //virtual void Serialize(XMLSerializer& serializer) = 0;
   //virtual void Deserialize(XMLSerializer& serializer) = 0;

   /**
    * Serialize this object to JSON.
    * 
    * @param serializer serializer to which the object is to be serialized
    */
   virtual void Serialize(JSONSerializer& serializer) = 0;

   /**
    * Deserialize this object from JSON.
    * 
    * @param serializer serializer from which the object is to be deserialized
    */
   virtual void Deserialize(JSONSerializer& serializer) = 0;

private:
   /**
    * Validate json in serializer
    * 
    * @param serializer serializer from which the json should be validated
   */
   virtual void Validate(JSONSerializer& serializer) {}

protected:
   /**
    * Validate json in serializer after serialize, by default calls this->Validate();
    * 
    * @param serializer serializer from which the json should be validated
   */
   virtual void Validate_after_serialize(JSONSerializer& serializer) { this->Validate(serializer); }
   
   /**
    * Validate json in serializer before deserialize, by default calls this->Validate();
    * 
    * @param serializer serializer from which the json should be validated
   */
   virtual void Validate_before_deserialize(JSONSerializer& serializer) { this->Validate(serializer); }

};

/**
 * Compile-time concepts to be used in (de)serialization code.
 */

/**
 * True if type inherits from JSONSerializable.
 */
template<typename T>
concept is_serializable = std::is_base_of_v<JSONSerializable, std::remove_cvref_t<T>>;

/**
 * True if the type if a buffer (pointer to unsigned char).
 */
template<typename T>
concept is_buffer = 
      std::is_pointer_v<std::remove_cvref_t<T>>
   && (std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, unsigned char>);

/**
 * True if the type if a buffer (pointer to unsigned char).
 */
//template<typename T>
//concept is_smart_pointer =
//      std::is_same_v<T, std::weak_ptr<T>>
//   || std::is_same_v<T, std::shared_ptr<T>>
//   || std::is_same_v<T, std::unique_ptr<T>>;
template <typename T> struct is_smart_ptr : std::false_type {};
template <typename T> struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T> struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
template <typename T> struct is_smart_ptr<std::weak_ptr<T>> : std::true_type {};
template <typename T> concept is_smart_pointer = is_smart_ptr<T>::value;

/**
 * True if the type is a variant of char.
 */
template<typename T>
concept is_char =
      std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, wchar_t>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char8_t>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char16_t>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char32_t>;

/**
 * True if the type is an array of variant of char.
 */
template<typename T>
concept is_char_array = 
   std::is_array_v<T> && is_char< std::remove_cvref_t<std::remove_extent_t<T>>>;


/**
 * True if the type is a CString or any variant of std::basic_string.
 */
template<typename T>
concept is_string = is_char<T>
   || is_char_array<T>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, CString>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::string>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::wstring>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::u8string>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::u16string>
   || std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::u32string>;

/**
 * True if the type is a CString or any variant of std::basic_string.
 */
template<typename T>
concept is_duration = std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::years>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::months>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::weeks>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::days>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::hours>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::minutes>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::seconds>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::milliseconds>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::microseconds>
|| std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, std::chrono::nanoseconds>;

/**
 * True if the type is a range as defined by standard.
 */
template<typename T>
concept is_collection = std::ranges::range<T>;

/**
 * Set of concepts to be True if the type is a std::array.
 */
template<class T> struct is_stdarray : std::is_array<T> {};
template<class T, std::size_t N> struct is_stdarray<std::array<T, N>> : std::true_type {};
// optional:
template<class T> struct is_stdarray<T const> : is_stdarray<T> {};
template<class T> struct is_stdarray<T volatile> : is_stdarray<T> {};
template<class T> struct is_stdarray<T volatile const> : is_stdarray<T> {};

template <typename Collection>
concept has_pushback = requires(Collection collection, 
   std::remove_cvref_t<std::ranges::range_value_t<Collection>> item) 
{
      {
         collection.push_back(item)
      } /*-> std::same_as<void>*/;
};


template <typename Collection>
concept has_insert = requires(Collection collection, 
   std::remove_cvref_t<std::ranges::range_value_t<Collection>>& item) 
{
   {
      collection.insert(item)
   } /*-> std::same_as<void>*/;
};


template <typename T>
concept has_allocation = requires(T instance, const nlohmann::json & _json)
{
   {
      T::AllocateFromJSON(_json)
   } -> std::same_as<T*>;
};

/**
 * Serializes a CString to JSON, converting cp1252 (the in-memory encoding) to
 * UTF-8 (the JSON wire contract). A string already holding valid UTF-8 (XML
 * content, DB reads) is passed through unchanged to avoid double-encoding.
 *
 * @param j JSON value receiving the string
 * @param c string to serialize
 */
COMMONTOOLS_EXPORT void to_json(nlohmann::json& j, const CString& c);

/**
 * Deserializes a JSON string into a CString, converting UTF-8 back to the
 * in-memory cp1252 convention (latin-1 range; longer sequences are kept as-is).
 *
 * @param j JSON value holding the string
 * @param c string receiving the result
 */
COMMONTOOLS_EXPORT void from_json(const nlohmann::json& j, CString& c);

/**
 * Class to serialize object to JSON.
 * 
 * The object to be serialized must implement the Serializable interface
 */
class COMMONTOOLS_EXPORT JSONSerializer
{
public:
   /**
    * Generated json object do be serialized to, or to deserialize from.
    */
   nlohmann::json m_json;

public:

   /**
    * Entry point stream operator to serialize the given object.
    * 
    * @param serializer instance of JSON serializer
    * @param obj instance of the object to serialize
    * @return the instance of JSON serializer
    */
   template <typename T>
   friend JSONSerializer& operator<<(JSONSerializer& serializer, const T& obj) {
      
      // String type (can be a char*, thus must be treated before considering pointer case)
      if constexpr (is_string<T>) {

         if constexpr (std::is_same_v<T, char>) {
            serializer.m_json = std::string(1, obj);
         } else {
            serializer.m_json = obj;
         }

      // Scalar and not a pointer
      } else if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
         serializer.m_json = obj;

      // Pointer to something, try to serialize the pointed value
      } else if constexpr (std::is_pointer_v<T>) {

         if (obj != nullptr) {

            // Call "Serialize" in case of Serializable object : give user code the chance 
            // to customize serialization
            if constexpr (is_serializable<std::remove_pointer_t<T>>) {
               obj->Serialize(serializer);

            // Normal serialization otherwise
            } else {
               serializer << *obj;
            }
         }
      // Pointer to something, try to serialize the pointed value
      } else if constexpr (is_smart_pointer<T>) {

         if (obj != nullptr) {

            // Call "Serialize" in case of Serializable object : give user code the chance 
            // to customize serialization
            if constexpr (is_serializable<typename T::element_type>) {
               obj->Serialize(serializer);

            // Normal serialization otherwise
            } else {
               serializer << *obj;
            }
         }

      // Object non "Serializable"-inherited and not a collection
      } else if constexpr (std::is_class_v<T> && !is_serializable<T> && !std::ranges::range<T>) {
         serializer.m_json = obj;

      // Pointer to unsigned char : we consider it is a buffer
      } else if constexpr (is_buffer<T>) {
         // TODO : Add base64 encoding
         serializer.m_json = (char*) obj;

      // "Serializable" value
      } else if constexpr (is_serializable<T>) {
         std::apply(
            [&] (auto&&... args) {
               ((serializer << std::as_const(args)), ...);
            },
            const_cast<T&>(obj).GetExposedMembers()
         );

      // Collections (vectors, lists, deque, map, unordered_map, set, unordered_set, PersistentArrays, etc)
      // TODO : also handle arrays ? (std::is_array)
      // If object is a collection
      } else if constexpr (std::ranges::range<T>) {

         serializer.m_json = nlohmann::json::array();

         for (auto& item : obj) {
            // TODO: Add enum, and simplify all that once reflexion has arrived - https://stackoverflow.com/a/28830941/5789813
            // TODO: Handle map types (unordered_map, unordered_set, map, set)

            JSONSerializer tmpSerializer;
            tmpSerializer << item;
            serializer.m_json.push_back(tmpSerializer.m_json);
         }
      }

      return serializer;
   }

   /**
    * Stream operator to serialize a particular member of a class.
    * 
    * @param serializer instance of JSON serializer
    * @param obj instance of the member to serialize
    * @return the instance of JSON serializer
    */
   template <typename Class, typename T>
   friend JSONSerializer& operator<<(JSONSerializer& serializer, const Member<Class, T>& member) {
      
      //TODO: Add enum, and simplify all that once reflexion has arrived - https://stackoverflow.com/a/28830941/5789813

      // String type (can be a char*, thus must be treated before considering pointer case)
      if constexpr (is_string<T>) {

         if constexpr (std::is_same_v<T, char>) {
            serializer.m_json[member.name] = std::string(1, member.GetValue());
         } else if constexpr (std::is_same_v<T, CString>) {
            // Route through to_json for the cp1252 -> UTF-8 boundary conversion.
            to_json(serializer.m_json[member.name], member.GetValue());
         } else {
            serializer.m_json[member.name] = std::string(member.GetValue());
         }
         return serializer;

      // chrono::duration
      } else if constexpr (is_duration<T>) {
         serializer.m_json[member.name] = member.GetValue().count();
         return serializer;

      // chrono::system_clock::time_point
      } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point, T>) {
         serializer.m_json[member.name] = std::chrono::system_clock::to_time_t(member.GetValue());
         return serializer;

      // Scalar and not a pointer
      } else if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
         serializer.m_json[member.name] = member.GetValue();
         return serializer;

      // Object non "Serializable"-inherited and not a collection
      } else if constexpr (std::is_class_v<T> && !is_serializable<T> && !std::ranges::range<T>) {
         serializer.m_json[member.name] = member.GetValue();
         return serializer;

      // Pointer to unsigned char : we consider it is a buffer
      } else if constexpr (is_buffer<T>) {
         // TODO : Add base64 encoding
         serializer.m_json[member.name] = (char*)member.GetValue();
         return serializer;

      // Pointer to something, try to serialize the pointed value
      } else if constexpr (std::is_pointer_v<T>) {

         if (member.GetValue() != nullptr) {

            // Call "Serialize" in case of Serializable object : give user code the chance 
            // to customize serialization
            if constexpr (is_serializable<std::remove_pointer_t<T>>) {
               JSONSerializer tmpSerializer;
               member.GetValue()->Serialize(tmpSerializer);
               serializer.m_json[member.name] = tmpSerializer.m_json;

            // Normal serialization otherwise
            } else {
               JSONSerializer tmpSerializer;
               tmpSerializer << *member.GetValue();
               serializer.m_json[member.name] = tmpSerializer.m_json;
            }
         }
         return serializer;

      // "Serializable" value
      } else if constexpr (is_serializable<T>) {

         JSONSerializer tmpSerializer;
         const_cast<T&>(member.GetValue()).Serialize(tmpSerializer);
         serializer.m_json[member.name] = tmpSerializer.m_json;
         return serializer;

      // Collections (vectors, lists, deque, map, unordered_map, set, unordered_set, PersistentArrays, etc)
      // TODO : also handle arrays ? (std::is_array)
      } else if constexpr (std::ranges::range<T>) {

         // TODO : Handle map types (unordered_map, unordered_set, map, set)
         JSONSerializer tmpSerializer;
         tmpSerializer << member.GetValue();
         serializer.m_json[member.name] = tmpSerializer.m_json;
         
         return serializer;
      } else {
         return serializer;
      }
   }

   /**
    * Entry point stream operator to deserialize the given object.
    *
    * @param serializer instance of JSON serializer
    * @param obj instance of the object to deserialize
    * @return the instance of JSON serializer
    */
   template <typename T>
   friend JSONSerializer& operator>>(JSONSerializer& serializer, T& obj) {
      std::apply(
         [&] (auto&&... args) {
            ((serializer >> args), ...);
         },
         const_cast<T&>(obj).GetExposedMembers()
      );
      return serializer;
   }

   /**
    * Stream operator to deserialize a particular member of a class.
    *
    * @param serializer instance of JSON serializer
    * @param obj instance of the member to deserialize
    * @return the instance of JSON serializer
    */
   template <typename Class, typename T>
   friend JSONSerializer& operator>>(JSONSerializer& serializer, Member<Class, T>& member) {

      if (!serializer.m_json.contains(member.name)) return serializer;

      // Do nothing if the target value is const, we can't change it.
      if constexpr (std::is_const_v<T>) {
         // noop
      // String type (can be a char*, thus must be treated before considering pointer case)
      } else if constexpr (is_string<T>) {

         // char, or wchar or char*_t
         if constexpr (is_char<T> && !std::is_pointer_v<T>) {
            member.SetValue( std::string(serializer.m_json[member.name])[0] );
         // std::*string
         } else if constexpr (
               std::is_same_v<T, std::string> 
            || std::is_same_v<T, std::wstring>
            || std::is_same_v<T, std::u8string>
            || std::is_same_v<T, std::u16string>
            || std::is_same_v<T, std::u32string>)
         {
            member.SetValue( serializer.m_json[member.name].template get<std::string>() );
         // non-const char * (const cannot be deserialized, only initialized)
         } else if constexpr (
               (is_char<T> && std::is_pointer_v<T> && !std::is_const_v<std::remove_pointer_t<T>>)
            || (is_char_array<T> && std::is_unbounded_array_v<T>) )
         {
            member.instance->*(member.ptr) = new std::remove_pointer_t<T>[serializer.m_json[member.name].template get<std::string>().size()];
            std::size_t buffer_size = serializer.m_json[member.name].template get<std::string>().length() + 1;
            char* deserialized_value = new char[buffer_size];
            memcpy(deserialized_value, serializer.m_json[member.name].template get<std::string>().c_str(), buffer_size);
            member.SetValue(deserialized_value);
         // char[X] or wchar_t[X] or other variants
         } else if constexpr (is_char_array<T> && std::is_bounded_array_v<T>) {
            std::size_t buffer_size = serializer.m_json[member.name].template get<std::string>().length() + 1;
            std::size_t target_size = std::extent_v<T> * sizeof(std::remove_extent_t<T>);
            char* destination = member.GetValue();
            memcpy(destination, serializer.m_json[member.name].template get<std::string>().c_str(), std::min(buffer_size, target_size));
         } else if constexpr (std::is_same_v<T, CString>) {
            CString converted_value;
            from_json(serializer.m_json[member.name], converted_value);
            member.SetValue(converted_value);
         }
         return serializer;

      // Scalar and not a pointer
      } else if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
         member.SetValue( serializer.m_json[member.name].template get<T>() );
         return serializer;

      // chrono::duration
      } else if constexpr (is_duration<T>) {
         member.SetValue(std::remove_cvref_t<T>(serializer.m_json[member.name]) );
			return serializer;
            
      // chrono::system_clock::time_point
      } else if constexpr (std::is_same_v<std::chrono::system_clock::time_point, T>) {
         member.SetValue(std::chrono::system_clock::from_time_t(serializer.m_json[member.name]));
			return serializer;
            
      // Object non "Serializable"-inherited and not a collection
      } else if constexpr (std::is_class_v<T> && !is_serializable<T> && !std::ranges::range<T>) {
         member.SetValue( serializer.m_json[member.name] );
         return serializer;

      // Pointer to unsigned char : we consider it is a buffer
      } else if constexpr (is_buffer<T>) {
         // TODO : Add base64 decoding
         //member.SetValue() = serializer.m_json[member.name];
         return serializer;

      // Pointer to something, try to deserialize the pointed value
      } else if constexpr (std::is_pointer_v<T>) {

         using U = std::remove_pointer_t<T>;

         if (serializer.m_json.contains(member.name)) {
            
            JSONSerializer tmpSerializer;
            tmpSerializer.m_json = serializer.m_json[member.name];

            U* new_value = nullptr;
            if constexpr (has_allocation<U>) {
               new_value = U::AllocateFromJSON(tmpSerializer.m_json);
            }
            else {
               new_value = new U();
            }

            // Call "Deserialize" in case of Serializable object : give user code the chance 
            // to customize serialization
            if constexpr (is_serializable<U>) {
               new_value->Deserialize(tmpSerializer);
            // Normal deserialization otherwise
            } else {
               tmpSerializer >> *new_value;
            }

            member.SetValue(new_value);
         }
         return serializer;

         // Pointer to something, try to deserialize the pointed value
      } else if constexpr (is_smart_pointer<T>) {

         using ItemType = std::remove_pointer_t<typename T::element_type>;

         if (serializer.m_json.contains(member.name)) {

            JSONSerializer tmpSerializer;
            tmpSerializer.m_json = serializer.m_json[member.name];

            T new_value = nullptr;
            if constexpr (has_allocation<ItemType>) {
               new_value = T(ItemType::AllocateFromJSON(tmpSerializer.m_json));
            }
            else {
               new_value = T(new ItemType());
            }

            // Call "Deserialize" in case of Serializable object : give user code the chance 
            // to customize serialization
            if constexpr (is_serializable<ItemType>) {
               new_value->Deserialize(tmpSerializer);
            // Normal deserialization otherwise
            } else {
               tmpSerializer >> *new_value;
            }

            member.SetValue(new_value);
         }
         return serializer;

      // "Serializable" value
      } else if constexpr (is_serializable<T>) {

         JSONSerializer tmpSerializer;
         tmpSerializer.m_json = serializer.m_json[member.name];
         T new_value = member.GetValue();
         new_value.Deserialize(tmpSerializer);
         member.SetValue(new_value);
         return serializer;

      // Collections (vectors, lists, deque, map, unordered_map, set, unordered_set, PersistentArrays, etc)
      } else if constexpr (std::ranges::range<T>) {

         // TODO : Handle map types (unordered_map, map)

         T new_range;

         using U = std::remove_cvref_t<std::ranges::range_value_t<std::remove_cvref_t<T>>>;

         // Pointer to sub-object
         if constexpr (std::is_pointer_v<U>) {

            using ItemType = std::remove_pointer_t<U>;

            int index = 0;

            for (auto& value : serializer.m_json[member.name]) {

               JSONSerializer tmpSerializer;
               tmpSerializer.m_json = value;

               ItemType tmpObject = nullptr;
               if constexpr (has_allocation<ItemType>) {
                  tmpObject = ItemType::AllocateFromJSON(tmpSerializer.m_json);
               }
               else {
                  tmpObject = new ItemType();
               }

               // Call "Deserialize" in case of Serializable object : give user code the chance 
               // to customize serialization
               if constexpr (is_serializable<U>) {
                  tmpObject->Deserialize(tmpSerializer);
               // Normal deserialization otherwise
               } else {
                  tmpSerializer >> *tmpObject;
               }

               // If std::array or c array
               if constexpr (is_stdarray<std::remove_reference_t<T>>::value
                  || std::is_array_v<std::remove_reference_t<T>>) {
                  new_range[index] = tmpObject;
                  index++;
               } else if constexpr (has_pushback<std::remove_reference_t<T>>) {
                  new_range.push_back(tmpObject);
               } else if constexpr (has_insert<std::remove_reference_t<T>>) {
                  new_range.insert(tmpObject);
               }
            }

         // Smart Pointer to sub-object
         } else if constexpr (is_smart_pointer<U>) {

            int index = 0;

            using ItemType = typename U::element_type;


            for (auto& value : serializer.m_json[member.name]) {

               JSONSerializer tmpSerializer;
               tmpSerializer.m_json = value;

               // TODO: allow InstanceAllocationFromJSON
               U tmpObject = nullptr;
               if constexpr (has_allocation<ItemType>) {
                  tmpObject = U(ItemType::AllocateFromJSON(tmpSerializer.m_json));
               }
               else {
                  tmpObject = U(new ItemType());
               }
               
               // Call "Deserialize" in case of Serializable object : give user code the chance 
               // to customize serialization

               // A smart pointer never inherits from JSONSerializable, we need to use ItemType to
               // deref the type from the smart pointer.
               if constexpr (is_serializable<ItemType>) {
                  tmpObject->Deserialize(tmpSerializer);
               // Normal deserialization otherwise
               } else {
                  tmpSerializer >> *tmpObject;
               }

               // If std::array or c array
               if constexpr (is_stdarray<std::remove_reference_t<T>>::value
                  || std::is_array_v<std::remove_reference_t<T>>) {
                  new_range[index] = tmpObject;
                  index++;
               } else if constexpr (has_pushback<std::remove_reference_t<T>>) {
                  new_range.push_back(tmpObject);
               } else if constexpr (has_insert<std::remove_reference_t<T>>) {
                  new_range.insert(tmpObject);
               }
            }

         // Subobject that need sub-deserialization
         } else if constexpr (is_serializable<U>) {

            int index = 0;

            for (auto& value : serializer.m_json[member.name]) {

               JSONSerializer tmpSerializer;
               tmpSerializer.m_json = value;
               U tmpObject;
               tmpObject.Deserialize(tmpSerializer);

               // If std::array or c array
               if constexpr (is_stdarray<std::remove_reference_t<T>>::value
                  || std::is_array_v<std::remove_reference_t<T>>) {
                  new_range[index] = tmpObject;
                  index++;
               } else if constexpr (has_pushback<std::remove_reference_t<T>>) {
                  new_range.push_back(tmpObject);
               } else if constexpr (has_insert<std::remove_reference_t<T>>) {
                  new_range.insert(tmpObject);
               }
            }

         // Simple value direct serializaion by nlohmann::json
         } else {

            int index = 0;

            for (auto& value : serializer.m_json[member.name]) {

               // If std::array or c array
               if constexpr (is_stdarray<std::remove_reference_t<T>>::value
                  || std::is_array_v<std::remove_reference_t<T>>) 
               {
                  new_range[index] = U(value);
                  index++;
               } else if constexpr (has_pushback<std::remove_reference_t<T>>) {
                  new_range.push_back(U(value));
               } else if constexpr (has_insert<std::remove_reference_t<T>>) {
                  new_range.insert(U(value));
               }
            }
         }

         member.SetValue(new_range);

         return serializer;
      } else {
         return serializer;
      }
   }

};

/**
 * Retrieve the JSON schema of a given type.
 * 
 * @return JSON schema of the given type
 */
template <typename T>
nlohmann::json GetSchema() {
   
   nlohmann::json result;

   // JSON types :
   //string
   //number
   //object
   //array
   //boolean

   // object: checked first so that serializable types win over implicit conversions
   // (e.g. types inheriting from MessageItem<N> have operator const char* but should
   // still be treated as objects when they implement JSONSerializable)
   if constexpr (is_serializable<std::remove_pointer_t<T>>) {
      result = nlohmann::json::object({ { "type", "object" } });
      result["properties"] = nlohmann::json::object();

      T obj;

      std::apply(
         [&] (auto&&... args) {
            (( result["properties"].update(GetMemberSchema(args)) ), ...);
         },
         obj.GetExposedMembers()
      );

   //string
   } else if constexpr (is_string<T> || is_buffer<T> || std::is_convertible_v<T, const char*>) {
      result = nlohmann::json::object({ { "type", "string" } });

   //enum
   } else if constexpr (std::is_enum_v<std::remove_pointer_t<T>>) {
      result = nlohmann::json::object({ { "type", "number" } });
      //TODO: Add enum reflexion once it's arrived - https://stackoverflow.com/a/28830941/5789813
      //result[member.name] = { "enum", [] };

   //number
   } else if constexpr ((std::is_floating_point_v<std::remove_pointer_t<T>> || std::is_integral_v<std::remove_pointer_t<T>>)
      && !std::is_same_v<T, bool>)
   {
      result = nlohmann::json::object({ { "type", "number" } });

   //boolean
   } else if constexpr (std::is_same_v<T, bool>) {
      result = nlohmann::json::object({ { "type", "boolean" } });

   //array : Collections (vectors, lists, deque, map, unordered_map, set, unordered_set, PersistentArrays, etc)
   } else if constexpr (std::ranges::range<T>) {
      result = nlohmann::json::object({
         { "type", "array" },
         { "items", GetSchema< typename T::value_type >() }
      });
   }

   return result;
}

/**
 * Retrieve the JSON schema of a given serializable type.
 *
 * @return JSON schema of the given type
 */
template<typename Class, typename T>
nlohmann::json GetMemberSchema(const Member<Class, T>& member) {
   nlohmann::json result = nlohmann::json::object();

   result[member.name] = GetSchema<std::remove_pointer_t<typename std::decay<T>::type > >();
   result[member.name]["description"] = member.description;

   return result;
}

namespace nlohmann {
	template <>
	struct adl_serializer<std::unique_ptr<char[]>> {
		static void to_json(nlohmann::json& j, const std::unique_ptr<char[]>& ptr) {
			j = ptr ? std::string(ptr.get()) : "";
		}

		static void from_json(const nlohmann::json& j, std::unique_ptr<char[]>& ptr) {
			std::string temp;
			j.get_to(temp);

			ptr = std::make_unique<char[]>(temp.size() + 1);
			std::strcpy(ptr.get(), temp.c_str());
		}	
	};
}

#pragma pop_macro("max")
#pragma pop_macro("min")
