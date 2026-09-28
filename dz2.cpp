#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <stdexcept>
#include <iomanip>
#include <ctime>

namespace Quantities {

    template <typename T>
    struct BaseQuantity {
        double value;

        explicit BaseQuantity(double v = 0.0) : value(v) {}

        friend std::ostream& operator<<(std::ostream& os, const T& q) {
            return os << q.value;
        }
    };

    struct Ampere : public BaseQuantity<Ampere> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_mA"; }
    };

    struct Volt : public BaseQuantity<Volt> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_V"; }
    };

    struct Ohm : public BaseQuantity<Ohm> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_Ohm"; }
    };

    struct Joule : public BaseQuantity<Joule> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_J"; }
    };

    struct Watt : public BaseQuantity<Watt> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_W"; }
    };

    struct Second : public BaseQuantity<Second> {
        using BaseQuantity::BaseQuantity;
        static const char* suffix() { return "_s"; }
    };

    inline Volt operator*(const Ampere& i, const Ohm& r) {
        return Volt{ i.value * r.value };
    }

    inline Volt operator*(const Ohm& r, const Ampere& i) {
        return Volt{ i.value * r.value };
    }

    inline Ampere operator/(const Volt& u, const Ohm& r) {
        if (r.value == 0) throw std::invalid_argument("Division by zero resistance");
        return Ampere{ u.value / r.value };
    }

    inline Watt operator*(const Volt& u, const Ampere& i) {
        return Watt{ u.value * i.value };
    }

    inline Watt operator*(const Ampere& i, const Volt& u) {
        return Watt{ u.value * i.value };
    }

    inline Joule operator*(const Watt& p, const Second& t) {
        return Joule{ p.value * t.value };
    }

    inline Joule operator*(const Second& t, const Watt& p) {
        return Joule{ p.value * t.value };
    }

    inline Volt operator/(const Watt& p, const Ampere& i) {
        if (i.value == 0) throw std::invalid_argument("Division by zero current");
        return Volt{ p.value / i.value };
    }

    inline Ampere operator/(const Watt& p, const Volt& u) {
        if (u.value == 0) throw std::invalid_argument("Division by zero voltage");
        return Ampere{ p.value / u.value };
    }
}

template <typename T>
struct Measurement {
    std::chrono::system_clock::time_point time;
    T value;
    T error;

    friend std::ostream& operator<<(std::ostream& os, const Measurement& m) {
        std::time_t t_c = std::chrono::system_clock::to_time_t(m.time);

        char time_str[26];
        ctime_s(time_str, sizeof(time_str), &t_c);

        std::string ts(time_str);
        if (!ts.empty() && ts.back() == '\n') ts.pop_back();

        os << "[" << ts << "] "
            << std::fixed << std::setprecision(2) << m.value.value << " " << T::suffix()
            << " (err: " << std::setprecision(3) << m.error.value << ")";
        return os;
    }
};

namespace Devices {

    class BaseDevice {
    protected:
        double range_min;
        double range_max;
        int accuracy_class;
        std::string id;

        mutable std::mt19937 gen;

    public:
        BaseDevice(double min, double max, int acc, std::string identifier)
            : range_min(min), range_max(max), accuracy_class(acc), id(std::move(identifier)),
            gen(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count())) {

            if (range_min >= range_max) {
                throw std::invalid_argument("Invalid range: min must be less than max");
            }
            if (accuracy_class <= 0) {
                throw std::invalid_argument("Accuracy class must be positive");
            }
        }

        virtual ~BaseDevice() = default;

        template <typename QuantityType>
        Measurement<QuantityType> measure() const {
            std::uniform_real_distribution<double> dist(range_min, range_max);
            double val = dist(gen);

            double err = (accuracy_class / 100.0) * (range_max - range_min);

            return Measurement<QuantityType>{
                std::chrono::system_clock::now(),
                    QuantityType{ val },
                    QuantityType{ err }
            };
        }

        std::string get_id() const { return id; }
    };

    class Voltmeter : public BaseDevice {
    public:
        using BaseDevice::BaseDevice;
        Measurement<Quantities::Volt> measure_voltage() const {
            return measure<Quantities::Volt>();
        }
    };

    class Ammeter : public BaseDevice {
    public:
        using BaseDevice::BaseDevice;
        Measurement<Quantities::Ampere> measure_current() const {
            return measure<Quantities::Ampere>();
        }
    };

    class Multimeter : public BaseDevice {
    public:
        using BaseDevice::BaseDevice;
        Measurement<Quantities::Volt> measure_voltage() const {
            return measure<Quantities::Volt>();
        }
        Measurement<Quantities::Ampere> measure_current() const {
            return measure<Quantities::Ampere>();
        }
    };
}

void demonstrate_quantities() {
    std::cout << "--- Quantities Demo ---" << std::endl;

    Quantities::Ampere i{ 2.0 };
    Quantities::Ohm r{ 5.0 };

    Quantities::Volt u = i * r;
    std::cout << "Voltage (I * R): " << u.value << " " << Quantities::Volt::suffix() << std::endl;

    Quantities::Watt p = u * i;
    std::cout << "Power (U * I): " << p.value << " " << Quantities::Watt::suffix() << std::endl;

    Quantities::Second t{ 10.0 };
    Quantities::Joule a = p * t;
    std::cout << "Work (P * t): " << a.value << " " << Quantities::Joule::suffix() << std::endl;

    Quantities::Ampere i2 = u / r;
    std::cout << "Current (U / R): " << i2.value << " " << Quantities::Ampere::suffix() << std::endl;
}

void demonstrate_devices() {
    std::cout << "\n--- Devices Demo ---" << std::endl;

    Devices::Voltmeter v(0.0, 250.0, 1, "V-001");
    Devices::Ammeter a(0.0, 10.0, 2, "A-001");
    Devices::Multimeter m(0.0, 1000.0, 5, "M-001");

    auto res_v = v.measure_voltage();
    auto res_a = a.measure_current();
    auto res_m = m.measure<Quantities::Volt>();

    std::cout << "Voltmeter result: " << res_v << "\n";
    std::cout << "Ammeter result: " << res_a << "\n";
    std::cout << "Multimeter result: " << res_m << "\n";

    try {
        Devices::Voltmeter bad_v(100.0, 50.0, 1, "BAD");
    }
    catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
}

int main() {
    demonstrate_quantities();
    demonstrate_devices();

    return 0;
}
