// TODO: omit zero values

#ifndef CPPXX_PROTO_PROTOBUF_H
#define CPPXX_PROTO_PROTOBUF_H

#include <cpp++/serde/tag_info.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <array>
#include <tuple>
#include <string>
#include <vector>
#include <unordered_map>

#ifndef GOOGLE_PROTOBUF_IO_CODED_STREAM_H__
#    include <google/protobuf/io/coded_stream.h>
#endif

#ifndef GOOGLE_PROTOBUF_IO_ZERO_COPY_STREAM_IMPL_LITE_H__
#    include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#endif

#ifndef GOOGLE_PROTOBUF_WIRE_FORMAT_LITE_H__
#    include <google/protobuf/wire_format_lite.h>
#endif

#ifndef BOOST_PFR_HPP
#    if __has_include(<boost/pfr.hpp>)
#        include <boost/pfr.hpp>
#    endif
#endif


namespace cppxx::proto::protobuf {
    template <typename From>
    using Serialize = ::cppxx::serde::Serialize<google::protobuf::io::CodedOutputStream, From>;

    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<google::protobuf::io::CodedInputStream, To>;

    using Dump  = ::cppxx::serde::Dump<google::protobuf::io::CodedOutputStream, std::string>;
    using Parse = ::cppxx::serde::Parse<google::protobuf::io::CodedInputStream, std::string>;

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val);

    template <typename T>
    void parse(const std::string &str, T &val);

    template <typename T>
    [[nodiscard]]
    T parse(const std::string &str);

    struct TagInfo {
        int              field_number = 0;
        bool             fixed        = false;
        bool             zigzag       = false;
        bool             packed       = true;
        std::string_view name         = "";
    };

    template <typename T>
    constexpr TagInfo get_tag_info(const T &field, std::string_view tag = "protobuf", char separator = ',') {
        TagInfo ti    = {};
        bool    first = true;

        std::string_view sv;
        if constexpr (is_tagged_v<T>)
            sv = field.get_tag(tag);

        while (!sv.empty()) {
            size_t           next = sv.find(separator);
            std::string_view part = sv.substr(0, next);

            if (first) {
                first   = false;
                ti.name = sv;
                for (size_t i = 0; i < sv.length(); ++i)
                    if (sv[i] >= '0' && sv[i] <= '9')
                        ti.field_number = ti.field_number * 10 + (sv[i] - '0');
                    else
                        break;
            } else if (part == "fixed")
                ti.fixed = true;
            else if (part == "zigzag")
                ti.zigzag = true;
            else if (part == "packed=false")
                ti.packed = false;
            else if (std::string_view n = "name="; part.size() >= n.size() && part.compare(0, n.size(), n) == 0)
                ti.name = part.substr(n.size());

            if (next == std::string_view::npos)
                break;
            sv.remove_prefix(next + 1);
        }

        return ti;
    }
} // namespace cppxx::proto::protobuf

namespace cppxx::proto::protobuf::detail {
    template <typename T>
    struct is_numeric : std::bool_constant<std::is_arithmetic_v<T> || std::is_enum_v<T>> {};


    template <typename T>
    struct is_bytes : std::false_type {};

    template <size_t N>
    struct is_bytes<std::array<uint8_t, N>> : std::true_type {};

    template <typename A>
    struct is_bytes<std::vector<uint8_t, A>> : std::true_type {};

    template <typename CT, typename A>
    struct is_bytes<std::basic_string<char, CT, A>> : std::true_type {};


    template <typename T>
    struct is_repeated_numeric : std::false_type {};

    template <typename T, size_t N>
    struct is_repeated_numeric<std::array<T, N>> : std::bool_constant<is_numeric<T>::value && !std::is_same_v<T, uint8_t>> {};

    template <typename T, typename A>
    struct is_repeated_numeric<std::vector<T, A>> : std::bool_constant<is_numeric<T>::value && !std::is_same_v<T, uint8_t>> {};


    template <typename T>
    struct is_repeated : std::false_type {};

    template <typename T, size_t N>
    struct is_repeated<std::array<T, N>> : std::bool_constant<!is_numeric<T>::value && !std::is_same_v<T, uint8_t>> {};

    template <typename T, typename A>
    struct is_repeated<std::vector<T, A>> : std::bool_constant<!is_numeric<T>::value && !std::is_same_v<T, uint8_t>> {};


    template <typename T>
    struct is_repeated_serializable : std::false_type {};

    template <typename T, size_t N>
    struct is_repeated_serializable<std::array<T, N>>
        : std::bool_constant<
              !is_repeated_numeric<std::array<T, N>>::value && !is_bytes<std::array<T, N>>::value &&
              serde::is_serializable_v<google::protobuf::io::CodedOutputStream, T>> {};

    template <typename T, typename A>
    struct is_repeated_serializable<std::vector<T, A>>
        : std::bool_constant<
              !is_repeated_numeric<std::vector<T, A>>::value && !is_bytes<std::vector<T, A>>::value &&
              serde::is_serializable_v<google::protobuf::io::CodedOutputStream, T>> {};


    template <typename T>
    struct is_repeated_deserializable : std::false_type {};

    template <typename T, size_t N>
    struct is_repeated_deserializable<std::array<T, N>>
        : std::bool_constant<
              !is_repeated_numeric<std::array<T, N>>::value && !is_bytes<std::array<T, N>>::value &&
              serde::is_deserializable_v<google::protobuf::io::CodedInputStream, T>> {};

    template <typename T, typename A>
    struct is_repeated_deserializable<std::vector<T, A>>
        : std::bool_constant<
              !is_repeated_numeric<std::vector<T, A>>::value && !is_bytes<std::vector<T, A>>::value &&
              serde::is_deserializable_v<google::protobuf::io::CodedInputStream, T>> {};


    template <typename T>
    struct is_message : std::false_type {};

    template <typename... Ts>
    struct is_message<std::tuple<Ts...>> : std::true_type {};


    template <typename T>
    struct is_std_array : std::false_type {};

    template <typename T, size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {};


    class DeserializeDispatcher {
    public:
        google::protobuf::io::CodedInputStream              &doc;
        google::protobuf::internal::WireFormatLite::WireType wire_type = {};
        size_t                                               len       = 0;
        bool                                                 fixed     = false;
        bool                                                 zigzag    = false;
        bool                                                 packed    = true;

        explicit DeserializeDispatcher(google::protobuf::io::CodedInputStream &doc) noexcept
            : doc(doc) {}

        virtual ~DeserializeDispatcher() = default;

        DeserializeDispatcher &set_wire_type(google::protobuf::internal::WireFormatLite::WireType wire_type) {
            this->wire_type = wire_type;
            return *this;
        }

        DeserializeDispatcher &set_fixed(bool fixed) {
            this->fixed = fixed;
            return *this;
        }

        DeserializeDispatcher &set_zigzag(bool zigzag) {
            this->zigzag = zigzag;
            return *this;
        }

        DeserializeDispatcher &set_packed(bool packed) {
            this->packed = packed;
            return *this;
        }

        DeserializeDispatcher &set_len(size_t len) {
            this->len = len;
            return *this;
        }

        DeserializeDispatcher &read_len() {
            if (wire_type == google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED)
                doc.ReadVarint64(&len);
            return *this;
        }

        virtual void read() = 0;
    };

    template <typename T>
    class DeserializeDispatcherFor : public DeserializeDispatcher {
    public:
        using DeserializeDispatcher::DeserializeDispatcher;

        DeserializeDispatcherFor(google::protobuf::io::CodedInputStream &doc, T &val) noexcept
            : proto::protobuf::detail::DeserializeDispatcher(doc)
            , val(&val) {}

        ~DeserializeDispatcherFor() override = default;

        T *val = nullptr;

        void into(T &val) {
            this->val = &val;
            read();
        }
    };
} // namespace cppxx::proto::protobuf::detail

namespace cppxx::serde {
    template <>
    struct Dump<google::protobuf::io::CodedOutputStream, std::string> {
        template <typename S>
        std::enable_if_t<proto::protobuf::detail::is_message<S>::value, std::string> from(const S &tpl) const {
            std::string                              buffer;
            google::protobuf::io::StringOutputStream os(&buffer);
            google::protobuf::io::CodedOutputStream  doc(&os);

            tuple_for_each(tpl, [&](const auto &v, size_t) {
                const proto::protobuf::TagInfo ti  = proto::protobuf::get_tag_info(v);
                const auto                    &val = serde::detail::get_underlying_value(v);
                using Tagged                       = std::decay_t<decltype(v)>;
                using T                            = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_serializable_v<google::protobuf::io::CodedOutputStream, T>)
                    Serialize<google::protobuf::io::CodedOutputStream, T>{doc}.from(val, ti);
            });

            return buffer;
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>, std::string> from(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            return from(tpl);
        }
#endif
    };

    template <>
    struct Parse<google::protobuf::io::CodedInputStream, std::string> {
        const std::string &buffer;

        template <typename... Ts>
        void into(std::tuple<Ts...> &tpl) const {
            google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());
            google::protobuf::io::CodedInputStream doc(&ais);

            std::array<std::unique_ptr<proto::protobuf::detail::DeserializeDispatcher>, sizeof...(Ts)> des = {};
            std::array<proto::protobuf::TagInfo, sizeof...(Ts)>                                        tis = {};

            tuple_for_each(tpl, [&](auto &v, size_t i) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_deserializable_v<google::protobuf::io::CodedInputStream, T>) {
                    tis[i] = proto::protobuf::get_tag_info(v);
                    des[i] = std::unique_ptr<proto::protobuf::detail::DeserializeDispatcher>(
                        new serde::Deserialize<google::protobuf::io::CodedInputStream, T>(doc, val)
                    );
                }
            });

            auto get_index = [&](int field_number) {
                size_t i = 0;
                for (; i < tis.size(); i++)
                    if (tis[i].field_number == field_number)
                        break;
                return i;
            };

            while (const uint32_t tag = doc.ReadTag()) {
                const uint32_t field_number = tag >> 3;
                const auto     wire_type    = static_cast<google::protobuf::internal::WireFormatLite::WireType>(tag & 0x07);
                const size_t   index        = get_index(field_number);

                try {
                    if (index >= sizeof...(Ts) || !des[index]) {
                        if (!google::protobuf::internal::WireFormatLite::SkipField(&doc, tag))
                            throw serde::error("cannot skip field");
                        continue;
                    }

                    auto &ser = *des[index];
                    ser.set_wire_type(wire_type)
                        .set_fixed(tis[index].fixed)
                        .set_zigzag(tis[index].zigzag)
                        .set_packed(tis[index].packed)
                        .read_len()
                        .read();
                } catch (serde::error &e) {
                    if (index >= sizeof...(Ts))
                        e.add_context(std::to_string(field_number));
                    else
                        e.add_context(tis[index].name);
                    throw;
                }
            }
            if (!doc.ConsumedEntireMessage())
                throw serde::error("message not fully consumed");
        }

        template <typename K, typename T, typename H, typename P, typename A>
        void into(std::unordered_map<K, T, H, P, A> &map) const {
            google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());
            google::protobuf::io::CodedInputStream doc(&ais);

            serde::Deserialize<google::protobuf::io::CodedInputStream, K> sk(doc);
            serde::Deserialize<google::protobuf::io::CodedInputStream, T> sv(doc);

            while (const uint32_t tag = doc.ReadTag()) {
                const uint32_t field_number = tag >> 3;
                const auto     wire_type    = static_cast<google::protobuf::internal::WireFormatLite::WireType>(tag & 0x07);

                K k;
                T v;
                try {
                    if (field_number == 1) {
                        sk.set_wire_type(wire_type).set_fixed(false).set_zigzag(false).set_packed(true).read_len();
                        sk.into(k);
                    } else if (field_number == 2) {
                        sv.set_wire_type(wire_type).set_fixed(false).set_zigzag(false).set_packed(true).read_len();
                        sv.into(v);
                    } else {
                        if (!google::protobuf::internal::WireFormatLite::SkipField(&doc, tag))
                            throw serde::error("cannot skip field");
                        continue;
                    }
                } catch (serde::error &e) {
                    e.add_context(std::to_string(field_number));
                    throw;
                }
                map.emplace(std::move(k), std::move(v));
            }
            if (!doc.ConsumedEntireMessage())
                throw serde::error("message not fully consumed");
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>> into(S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            into(tpl);
        }
#endif
    };

    // numeric
    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_numeric<T>::value>> {
        google::protobuf::io::CodedOutputStream &doc;
        mutable bool                             fixed  = false;
        mutable bool                             zigzag = false;

        void from(T v, const proto::protobuf::TagInfo &ti) const {
            auto tag = google::protobuf::internal::WireFormatLite::MakeTag(
                ti.field_number,
                std::is_same_v<T, double>       ? google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64
                : std::is_same_v<T, float>      ? google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32
                : !ti.fixed                     ? google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT
                : sizeof(T) == sizeof(uint64_t) ? google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64
                                                : google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32
            );
            this->doc.WriteTag(tag);
            fixed  = ti.fixed;
            zigzag = ti.zigzag;
            from(v);
        }

        void from(const T &v) const {
            if constexpr (std::is_same_v<T, double>) {
                this->doc.WriteLittleEndian64(google::protobuf::internal::WireFormatLite::EncodeDouble(v));
            } else if constexpr (std::is_same_v<T, float>) {
                this->doc.WriteLittleEndian32(google::protobuf::internal::WireFormatLite::EncodeFloat(v));
            } else {
                if (fixed) {
                    if constexpr (sizeof(T) == 8)
                        this->doc.WriteLittleEndian64(static_cast<uint64_t>(v));
                    else
                        this->doc.WriteLittleEndian32(static_cast<uint32_t>(v));
                } else if (zigzag) {
                    if constexpr (sizeof(T) == 8)
                        this->doc.WriteVarint64(google::protobuf::internal::WireFormatLite::ZigZagEncode64(static_cast<int64_t>(v)
                        ));
                    else
                        this->doc.WriteVarint32(google::protobuf::internal::WireFormatLite::ZigZagEncode32(static_cast<int32_t>(v)
                        ));
                } else {
                    if constexpr (sizeof(T) == 8)
                        this->doc.WriteVarint64(static_cast<uint64_t>(v));
                    else
                        this->doc.WriteVarint32(static_cast<uint32_t>(v));
                }
            }
        }
    };

    template <typename T>
    struct Deserialize<google::protobuf::io::CodedInputStream, T, std::enable_if_t<proto::protobuf::detail::is_numeric<T>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<T> {

        using proto::protobuf::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            if (this->wire_type == google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
                uint32_t bits;
                if (!this->doc.ReadLittleEndian32(&bits))
                    throw serde::error("failed to decode int");
                if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
                    *this->val = (T)google::protobuf::internal::WireFormatLite::DecodeFloat(bits);
                } else {
                    if (this->zigzag)
                        *this->val = static_cast<T>(google::protobuf::internal::WireFormatLite::ZigZagDecode32(bits));
                    else
                        *this->val = static_cast<T>(bits);
                }
            } else {
                uint64_t bits;
                if (!(this->wire_type == google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64
                          ? this->doc.ReadLittleEndian64(&bits)
                          : this->doc.ReadVarint64(&bits)))
                    throw serde::error("failed to decode int");

                if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
                    *this->val = (T)google::protobuf::internal::WireFormatLite::DecodeDouble(bits);
                } else {
                    if (this->zigzag)
                        *this->val = static_cast<T>(google::protobuf::internal::WireFormatLite::ZigZagDecode64(bits));
                    else
                        *this->val = static_cast<T>(bits);
                }
            }
        }
    };

    // bytes and string
    template <typename T>
    struct Serialize<google::protobuf::io::CodedOutputStream, T, std::enable_if_t<proto::protobuf::detail::is_bytes<T>::value>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const T &v, const proto::protobuf::TagInfo &ti) const {
            auto tag = google::protobuf::internal::WireFormatLite::MakeTag(
                ti.field_number, google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED
            );
            doc.WriteTag(tag);
            doc.WriteVarint64(v.size());
            from(v);
        }

        void from(const T &v) const {
            doc.WriteRaw(v.data(), (int)v.size());
        }
    };

    template <typename T>
    struct Deserialize<google::protobuf::io::CodedInputStream, T, std::enable_if_t<proto::protobuf::detail::is_bytes<T>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<T> {

        using proto::protobuf::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            if (this->wire_type != google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED)
                throw serde::error("failed to deserialize");

            bool ok = false;
            if constexpr (std::is_same_v<T, std::string>) {
                ok = this->doc.ReadString(this->val, (int)this->len);
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                this->val->resize(this->len);
                ok = this->doc.ReadRaw(this->val->data(), (int)this->len);
            } else {
                ok = this->doc.ReadRaw(this->val->data(), (int)this->len);
            }

            if (!ok)
                throw serde::error("failed to deserialize");
        }
    };

    // repeated numeric
    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_repeated_numeric<T>::value>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const T &arr, const proto::protobuf::TagInfo &ti) const {
            if (!ti.packed) {
                Serialize<google::protobuf::io::CodedOutputStream, typename T::value_type> ser = {this->doc};
                for (auto &v : arr)
                    ser.from(v, ti);
            } else {
                std::string buffer = create_packed_buffer(arr, ti);
                Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer, ti);
            }
        }

        void from(const T &arr) const {
            std::string buffer = create_packed_buffer(arr);
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer);
        }

        std::string create_packed_buffer(const T &arr, const proto::protobuf::TagInfo &ti = {}) const {
            std::string                              buffer;
            google::protobuf::io::StringOutputStream os(&buffer);
            google::protobuf::io::CodedOutputStream  doc(&os);

            Serialize<google::protobuf::io::CodedOutputStream, typename T::value_type> ser = {doc};

            ser.fixed  = ti.fixed;
            ser.zigzag = ti.zigzag;
            for (auto &v : arr)
                ser.from(v);

            return buffer;
        }
    };

    template <typename T>
    struct Deserialize<
        google::protobuf::io::CodedInputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_repeated_numeric<T>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<T> {

        using proto::protobuf::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        size_t i = 0;

        void read() override {
            if (this->packed) {
                read_packed();
                return;
            }

            Deserialize<google::protobuf::io::CodedInputStream, typename T::value_type> des(this->doc);
            des.set_wire_type(this->wire_type).set_fixed(this->fixed).set_zigzag(this->zigzag);

            T &arr = *this->val;
            if constexpr (proto::protobuf::detail::is_std_array<T>::value) {
                des.into(arr[i++]);
            } else {
                typename T::value_type val;
                des.into(val);
                arr.push_back(val);
            }
        }

        void read_packed() {
            Deserialize<google::protobuf::io::CodedInputStream, std::string> ser(this->doc);
            ser.set_wire_type(this->wire_type).set_len(this->len);
            std::string buffer;
            ser.into(buffer);

            google::protobuf::io::ArrayInputStream ais(buffer.data(), (int)buffer.size());
            google::protobuf::io::CodedInputStream doc(&ais);
            doc.PushLimit((int)buffer.size());

            Deserialize<google::protobuf::io::CodedInputStream, typename T::value_type> des(doc);

            bool fixed =
                std::is_same_v<typename T::value_type, double> || std::is_same_v<typename T::value_type, float> || this->fixed;

            auto wire_type = !fixed                                ? google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT
                             : sizeof(typename T::value_type) == 8 ? google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64
                                                                   : google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32;
            des.set_wire_type(wire_type).set_fixed(fixed).set_zigzag(this->zigzag);

            for (T &arr = *this->val; doc.BytesUntilLimit();) {
                if constexpr (proto::protobuf::detail::is_std_array<T>::value) {
                    des.into(arr.at(i++));
                } else {
                    typename T::value_type val;
                    des.into(val);
                    arr.push_back(val);
                }
            }
        }
    };

    // optional
    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        std::optional<T>,
        std::enable_if_t<is_serializable_v<google::protobuf::io::CodedOutputStream, T>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const std::optional<T> &v, const proto::protobuf::TagInfo &ti) const {
            if (v.has_value())
                Serialize<google::protobuf::io::CodedOutputStream, T>{doc}.from(*v, ti);
        }

        void from(const std::optional<T> &v) const {
            if (v.has_value())
                Serialize<google::protobuf::io::CodedOutputStream, T>{doc}.from(*v);
        }
    };

    template <typename T>
    struct Deserialize<
        google::protobuf::io::CodedInputStream,
        std::optional<T>,
        std::enable_if_t<is_deserializable_v<google::protobuf::io::CodedInputStream, T>>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<std::optional<T>> {

        using proto::protobuf::detail::DeserializeDispatcherFor<std::optional<T>>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            if (!this->val->has_value())
                *this->val = T{};

            Deserialize<google::protobuf::io::CodedInputStream, T> ser(this->doc);
            ser.set_wire_type(this->wire_type)
                .set_fixed(this->fixed)
                .set_zigzag(this->zigzag)
                .set_packed(this->packed)
                .set_len(this->len);
            ser.into(this->val->value());
        }
    };

    // repeated
    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_repeated_serializable<T>::value>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const T &arr, const proto::protobuf::TagInfo &ti) const {
            for (const auto &v : arr)
                Serialize<google::protobuf::io::CodedOutputStream, std::decay_t<decltype(v)>>{doc}.from(v, ti);
        }

        void from(const T &arr) const {
            for (const auto &v : arr)
                Serialize<google::protobuf::io::CodedOutputStream, std::decay_t<decltype(v)>>{doc}.from(v);
        }
    };

    template <typename T>
    struct Deserialize<
        google::protobuf::io::CodedInputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_repeated_deserializable<T>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<T> {

        using proto::protobuf::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        size_t i = 0;

        void read() override {
            Deserialize<google::protobuf::io::CodedInputStream, typename T::value_type> ser(this->doc);
            ser.set_wire_type(this->wire_type)
                .set_fixed(this->fixed)
                .set_zigzag(this->zigzag)
                .set_packed(this->packed)
                .set_len(this->len);

            T &arr = *this->val;
            if constexpr (proto::protobuf::detail::is_std_array<T>::value) {
                ser.into(arr.at(i++));
            } else {
                arr.emplace_back();
                ser.into(arr.back());
            }
        }
    };

    // message
    template <typename T>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        T,
        std::enable_if_t<proto::protobuf::detail::is_message<T>::value>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const T &tpl, const proto::protobuf::TagInfo &ti) {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(tpl);
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer, ti);
        }

        void from(const T &tpl) {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(tpl);
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer);
        }
    };

    template <typename T>
    struct Deserialize<google::protobuf::io::CodedInputStream, T, std::enable_if_t<proto::protobuf::detail::is_message<T>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<T> {

        using proto::protobuf::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            Deserialize<google::protobuf::io::CodedInputStream, std::string> ser(this->doc);
            ser.set_wire_type(this->wire_type).set_len(this->len);
            std::string buffer;
            ser.into(buffer);
            Parse<google::protobuf::io::CodedInputStream, std::string>{buffer}.into(*this->val);
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        S,
        std::enable_if_t<std::is_aggregate_v<S> && !proto::protobuf::detail::is_repeated<S>::value>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const S &v, const proto::protobuf::TagInfo &ti) {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(v);
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer, ti);
        }

        void from(const S &v) {
            std::string buffer = Dump<google::protobuf::io::CodedOutputStream, std::string>{}.from(v);
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer);
        }
    };

    template <typename S>
    struct Deserialize<
        google::protobuf::io::CodedInputStream,
        S,
        std::enable_if_t<std::is_aggregate_v<S> && !proto::protobuf::detail::is_repeated<S>::value>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<S> {

        using proto::protobuf::detail::DeserializeDispatcherFor<S>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            Deserialize<google::protobuf::io::CodedInputStream, std::string> ser(this->doc);
            ser.set_wire_type(this->wire_type).set_len(this->len);
            std::string buffer;
            ser.into(buffer);
            Parse<google::protobuf::io::CodedInputStream, std::string>{buffer}.into(*this->val);
        }
    };
#endif

    template <typename K, typename T, typename H, typename P, typename A>
    struct Serialize<
        google::protobuf::io::CodedOutputStream,
        std::unordered_map<K, T, H, P, A>,
        std::enable_if_t<
            is_serializable_v<google::protobuf::io::CodedOutputStream, K> &&
            is_serializable_v<google::protobuf::io::CodedOutputStream, T>>> {
        google::protobuf::io::CodedOutputStream &doc;

        void from(const std::unordered_map<K, T, H, P, A> &v, const proto::protobuf::TagInfo &ti) {
            std::string buffer;
            {
                google::protobuf::io::StringOutputStream os(&buffer);
                google::protobuf::io::CodedOutputStream  doc(&os);
                Serialize<google::protobuf::io::CodedOutputStream, std::unordered_map<K, T, H, P, A>>{doc}.from(v);
            }
            Serialize<google::protobuf::io::CodedOutputStream, std::string>{this->doc}.from(buffer, ti);
        }

        void from(const std::unordered_map<K, T, H, P, A> &map) {
            proto::protobuf::TagInfo tk = {1}, tv = {2};
            for (auto &[k, v] : map) {
                Serialize<google::protobuf::io::CodedOutputStream, K>{this->doc}.from(k, tk);
                Serialize<google::protobuf::io::CodedOutputStream, T>{this->doc}.from(v, tv);
            }
        }
    };

    template <typename K, typename T, typename H, typename P, typename A>
    struct Deserialize<
        google::protobuf::io::CodedOutputStream,
        std::unordered_map<K, T, H, P, A>,
        std::enable_if_t<
            is_deserializable_v<google::protobuf::io::CodedInputStream, K> &&
            is_deserializable_v<google::protobuf::io::CodedInputStream, T>>>
        : public proto::protobuf::detail::DeserializeDispatcherFor<std::unordered_map<K, T, H, P, A>> {

        using proto::protobuf::detail::DeserializeDispatcherFor<std::unordered_map<K, T, H, P, A>>::DeserializeDispatcherFor;
        ~Deserialize() override = default;

        void read() override {
            Deserialize<google::protobuf::io::CodedInputStream, std::string> ser(this->doc);
            ser.set_wire_type(this->wire_type).set_len(this->len);
            std::string buffer;
            ser.into(buffer);

            Parse<google::protobuf::io::CodedInputStream, std::string>{buffer}.into(*this->val);
        }
    };
} // namespace cppxx::serde

namespace cppxx::proto::protobuf {
    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val) {
        return Dump{}.from(val);
    }

    template <typename T>
    void parse(const std::string &str, T &val) {
        Parse{str}.into(val);
    }

    template <typename T>
    [[nodiscard]]
    T parse(const std::string &str) {
        T val;
        Parse{str}.into(val);
        return val;
    }
} // namespace cppxx::proto::protobuf
#endif
