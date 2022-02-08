#ifndef PROPERTY_CLASS_H
#define PROPERTY_CLASS_H

#include <QRect>
#include <QSize>
#include <QVariant>
#include <functional>
#include <cmath>
#include <utility>

namespace FunctionDef {
using Supported = std::function<bool()>;
using Get = std::function<QVariant()>;
using Set = std::function<int(const QVariant&)>;
using Range = std::function<int(QVariant&,QVariant&,QVariant&)>;
};


class Property : public QObject
{
    Q_OBJECT
public:
    Property()
    {
        m_value = 0; //GENERIC!
        supported = false;
    }

    explicit Property(QVariant value)
    {
        m_value = std::move(value);
    }

    explicit Property(bool value)
    {
        m_value = value;
    }

    explicit Property(int value)
    {
        m_value = value;
    }

    explicit Property(unsigned int value)
    {
        m_value = value;
    }

    explicit Property(double value)
    {
        m_value = value;
    }

    ~Property() override = default;

    virtual QVariant value() const
    {
        return m_value;
    }

    virtual int setValue(const QVariant& value)
    {
        int ret = 0;
        if(m_value != value)
        {
            m_value = value;

            if (!signalsBlocked())
            {
                ret = sync();
                emit valueChanged(m_value);
            }
        }
        return ret;
    }

    void setFuncGet(FunctionDef::Get f)
    {
        m_getValueFunc = std::move(f);
    }

    void setFuncSet(FunctionDef::Set f)
    {
        m_setValueFunc = std::move(f);
    }

    void setFuncSupported(FunctionDef::Supported f)
    {
        m_getSupportedFunc = std::move(f);
    }

    virtual void update()
    {
        if(m_getSupportedFunc)
        {
            auto val = m_getSupportedFunc();
            setSupported(val);
        }

        if(m_getValueFunc)
        {
            auto val = m_getValueFunc();
            setValue(val);
        }
    }

    int sync()
    {
        if(m_setValueFunc)
        {
            return m_setValueFunc(m_value);
        }
        return 0;
    }

    QVariant operator[](const QString& s) const
    {
        if(s == "value")
        {
            return value();
        }
        else
        {
            return {};
        }
    }

    virtual Property& operator=(QVariant& v)
    {
        setValue(v);
        return *this;
    }

    Property& operator=(int& v)
    {
        QVariant value = v;
        setValue(value);
        return *this;
    }

    Property& operator=(unsigned int& v)
    {
        QVariant value = v;
        setValue(value);
        return *this;
    }

    virtual Property& operator=(bool v)
    {
        QVariant value = v;
        setValue(value);
        return *this;
    }

    Property& operator=(double& v)
    {
        QVariant value = v;
        setValue(value);
        return *this;
    }

    QVariant operator()() const
    {
        return value();
    }

    bool isSupported()
    {
        return supported;
    }

signals:
    void valueChanged(QVariant value);

private:
    QVariant m_value;
    bool supported = false;
    FunctionDef::Get m_getValueFunc;
    FunctionDef::Supported m_getSupportedFunc;
    FunctionDef::Set m_setValueFunc;

    void setSupported(bool s)
    {
        this->supported = s;
    }
};

class PropertyRange : public Property
{
    Q_OBJECT
    public:
        PropertyRange() : Property()
        {
            has_range = false;
        }

        PropertyRange(QVariant value, QVariant min, QVariant max, QVariant inc) : Property(std::move(value))
        {
            m_min = std::move(min);
            m_max = std::move(max);
            m_inc = std::move(inc);
            has_range = true;
        }

        ~PropertyRange() override = default;

        /**
         * @brief taken from https://embeddeduse.com/2019/08/26/qt-compare-two-floats/
         */
        static bool doubleCompare(double f1, double f2, double epsilon)
        {
            auto abs = qAbs(f1 - f2);
            if (abs <= epsilon)
                return true;

            auto otherVal = epsilon * qMax(qAbs(f1), qAbs(f2));
            return abs <= otherVal;
        }

        int setValue(const QVariant& value) override
        {
            auto val = value;
            if(m_additionalAllowedValues.contains(val))
            {
                // OK
            }
            else if(has_range)
            {
                if(m_checkInc)
                {
                    switch(val.type())
                    {
                    case QVariant::Type::Int:
                    case QVariant::Type::UInt:
                    case QVariant::Type::LongLong:
                    case QVariant::Type::ULongLong:
                    {
                        if(m_inc.toLongLong() != 0 && (val.toLongLong() % m_inc.toLongLong()) != 0)
                        {
                            long long tmp = val.toLongLong();
                            tmp -= tmp % m_inc.toLongLong();
                            val = tmp;
                        }
                        break;
                    }
                    case QVariant::Type::Double:
                    {
                        // ensure epsilon << 1, as otherwise this makes no sense!
                        auto epsilon = (m_inc.toDouble() != 0 && m_inc.toDouble() < 1.0) ? m_inc.toDouble() : 0.01f;
                        if (doubleCompare(val.toDouble(), this->value().toDouble(), epsilon))
                        {
                            return 0;
                        }

                        if (m_inc.toDouble() != 0 && std::fmod(val.toDouble(), m_inc.toDouble()) != 0)
                        {
                            double temp = val.toDouble();
                            temp -=std::fmod(temp, m_inc.toDouble());
                            val = temp;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }

                if(val.toDouble() < m_min.toDouble())
                {
                    val = m_min;
                }
                else if(val.toDouble() > m_max.toDouble())
                {
                    val = m_max;
                }
            }
            return Property::setValue(val);
        }

        void setRange(const QVariant& min, const QVariant& max, const QVariant& inc)
        {
            if(m_min != min || m_max != max || m_inc != inc)
            {
                m_min = min;
                m_max = max;
                m_inc = inc;
                has_range = true;

                emit rangeChanged(min, max, inc);
            }

        }

        bool hasValidRange()
        {
            if(has_range)
            {
                if(m_min != m_max && m_min.isValid())
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        QVariantList range()
        {
            return {m_min, m_max, m_inc};
        }

        QVariant min()
        {
            return m_min;
        }

        QVariant max()
        {
            return m_max;
        }

        QVariant inc()
        {
            return m_inc;
        }

        void setRangeFunc(FunctionDef::Range f)
        {
            m_getRangeValueFunc = f;
        }

        void update() override
        {
            QVariant min, max, inc;
            if(m_getRangeValueFunc)
            {
                m_getRangeValueFunc(min, max, inc);
                setRange(min, max, inc);
            }

            Property::update();
        }

        void validateInc(bool enabled)
        {
            m_checkInc = enabled;
        }

        QVariant operator[](const QString& s)
        {
            if(s == "min")
            {
                return m_min;
            }
            else if(s == "max")
            {
                return m_max;
            }
            else if(s == "inc")
            {
                return m_inc;
            }
            else
            {
                return Property::operator[](s);
            }
        }

        void addRangeException(const QVariant& value)
        {
            if(!m_additionalAllowedValues.contains(value))
            {
                m_additionalAllowedValues.append(value);

            }
        }

        void clearRangeException()
        {
            m_additionalAllowedValues.clear();
        }

    signals:
        void rangeChanged(QVariant min, QVariant max, QVariant inc);

    private:
        QVariant m_min;
        QVariant m_max;
        QVariant m_inc;
        bool has_range = false;
        bool m_checkInc = true;
        QVariantList m_additionalAllowedValues;

        std::function<bool(QVariant&, QVariant&, QVariant&)> m_getRangeValueFunc;
};

class PropertyBool : public Property
{
public:
    PropertyBool() : Property()
    {

    }
    explicit PropertyBool(bool p) : Property(p)
    {

    }

    PropertyBool& operator=(bool v) override
    {
        QVariant value = v;
        setValue(value);
        return *this;
    }

    PropertyBool& operator=(QVariant &v) override
    {
        setValue(v);
        return *this;
    }

    bool operator()() const // ignore warning here
    {
        return value().toBool();
    }

};

class PropertySize : public QObject
{
    Q_OBJECT
public:
    using GetFuncDef = std::function<QRect()>;
    using SetFuncDef = std::function<bool(QRect)>;
    using SizeRangeFuncDef = std::function<bool(QSize&,QSize&,QSize&)>;
    using SizePosFuncDef = std::function<bool(QPoint&,QPoint&,QPoint&)>;

    PropertySize() = default;

    explicit PropertySize(QSize s)
    {
        m_rect.setSize(s);
    }

    explicit PropertySize(QRect s)
    {
        m_rect = s;
    }

    void setSize(QSize size)
    {
        if(m_rect.size() != size)
        {
            m_rect.setSize(size);
            sync();
            emit sizeChanged();
            emit rectChanged(m_rect);
        }
    }

    QSize size()
    {
        return m_rect.size();
    }

    QSize minSize()
    {
        return m_minSize;
    }

    QSize maxSize()
    {
        return m_maxSize;
    }

    QSize incSize()
    {
        return m_incSize;
    }

    void setPos(QPoint pt)
    {
        if(m_rect.topLeft() != pt)
        {
            auto oldRect = m_rect;
            // setTopLeft changes rect size
            m_rect.setRect(pt.x(), pt.y(), oldRect.width(), oldRect.height());
            sync();
            emit posChanged();
            emit rectChanged(m_rect);
        }
    }

    QPoint pos()
    {
        return m_rect.topLeft();
    }

    QPoint minPos()
    {
        return m_minPos;
    }

    QPoint maxPos()
    {
        return m_maxPos;
    }

    QPoint incPos()
    {
        return m_incPos;
    }


    void setRect(QRect rect)
    {
        bool bPosChanged = rect.topLeft() != m_rect.topLeft();
        bool bSizeChanged = rect.size() != m_rect.size();

        if(bPosChanged || bSizeChanged)
        {
            m_rect = rect;
            sync();
            emit rectChanged(m_rect);
        }

        if(bPosChanged)
        {
            emit posChanged();
        }
        if(bSizeChanged)
        {
            emit sizeChanged();
        }
    }

    QRect rect()
    {
        return m_rect;
    }

    void setFuncGet(GetFuncDef f)
    {
        m_getValueFunc = std::move(f);
    }

    void setFuncSet(SetFuncDef f)
    {
        m_setValueFunc = std::move(f);
    }

    void update()
    {
        if(m_getValueFunc)
        {
            auto val = m_getValueFunc();
            setRect(val);
        }
    }

    void sync()
    {
        if(m_setValueFunc)
        {
            m_setValueFunc(rect());
        }
    }

    void setSizeRangeFunc(SizeRangeFuncDef func)
    {
        m_setRangeSizeFunc = std::move(func);
    }

    void setSizePosFunc(SizePosFuncDef func)
    {
        m_setRangePosFunc = std::move(func);
    }

    void updateRanges()
    {
        updateSizeRange();
        updatePosRange();
    }

    void updateSizeRange()
    {
        QSize min, max, inc;

        if(m_setRangeSizeFunc(min, max, inc))
        {
            m_minSize = min;
            m_maxSize = max;
            m_incSize = inc;

            emit sizeRangeChanged();
        }
    }

    void updatePosRange()
    {
        QPoint min, max, inc;

        if(m_setRangePosFunc(min, max, inc))
        {
            m_minPos = min;
            m_maxPos = max;
            m_incPos = inc;

            emit posRangeChanged();
        }
    }

    void updateAll()
    {
        update();
        updateRanges();
    }


    PropertySize& operator=(QRect& v)
    {
        setRect(v);
        return *this;
    }

    QRect operator()()
    {
        return rect();
    }

signals:
    void sizeChanged();
    void posChanged();
    void rectChanged(QRect);

    void sizeRangeChanged();
    void posRangeChanged();

private:
    QRect m_rect;

    QSize m_minSize;
    QSize m_maxSize;
    QSize m_incSize;

    QPoint m_minPos;
    QPoint m_maxPos;
    QPoint m_incPos;

    GetFuncDef m_getValueFunc;
    SetFuncDef m_setValueFunc;
    SizeRangeFuncDef m_setRangeSizeFunc;
    SizePosFuncDef m_setRangePosFunc;

};


class PropertyRangeList: public QObject
{
    Q_OBJECT
public:
    PropertyRangeList() : QObject()
    {

    }

    QVariant value(int listIndex)
    {
        if(listIndex < m_valueList.size())
        {
            return m_valueList[listIndex];
        }
        else
        {
            return {};
        }
    }

    int size()
    {
        return m_valueList.size();
    }

    QVariantList min()
    {
        return m_rangeMinList;
    }

    QVariantList max()
    {
        return m_rangeMaxList;
    }

    QVariantList inc()
    {
        return m_rangeIncList;
    }

    void update()
    {
        if(m_getValFunc)
        {
            QVariantList a;
            if(m_getValFunc(a))
            {
                if(m_valueList != a)
                {
                    m_valueList = a;
                    emit valuesChanged();
                }
            }
        }
        if(m_getRangeFunc)
        {
            QVariantList min, max, inc;
            if(m_getRangeFunc(min, max, inc))
            {
                if(m_rangeMinList != min || m_rangeMaxList != max || m_rangeIncList != inc )
                {
                    m_rangeIncList = inc;
                    m_rangeMaxList = max;
                    m_rangeMinList = min;

                    emit rangesChanged();
                }

            }
        }
    }

    void setFuncGet(std::function<bool(QVariantList&)> f)
    {
        m_getValFunc = std::move(f);
    }

    void setFuncSet(std::function<bool(QVariantList&)> f)
    {
        m_setValFunc = std::move(f);
    }

    void setFuncRange(std::function<bool(QVariantList&, QVariantList&, QVariantList&)> f)
    {
        m_getRangeFunc = std::move(f);
    }

public slots:
    void setValue(int listIndex, const QVariant& value)
    {
        if(listIndex < m_valueList.size())
        {
            m_valueList.replace(listIndex, value);
        }
        else
        {
            //TODO: Insert?
        }

    }

    void sync()
    {
        if(m_setValFunc)
        {
            m_setValFunc(m_valueList);
        }
    }

signals:
    void valuesChanged();
    void rangesChanged();


private:
    QVariantList m_valueList;
    QVariantList m_rangeMinList;
    QVariantList m_rangeMaxList;
    QVariantList m_rangeIncList;

    std::function<bool(QVariantList&)> m_getValFunc;
    std::function<bool(QVariantList&)> m_setValFunc;
    std::function<bool(QVariantList&, QVariantList&, QVariantList&)> m_getRangeFunc;
};


class PropertyPwm: public PropertyRangeList
{
    Q_OBJECT
public:
    PropertyPwm() : PropertyRangeList()
    {

    }

    double freq()
    {
        return value(0).toDouble();
    }

    double duty()
    {
        return value(1).toDouble();
    }

public slots:
    void setFreq(const QVariant& freq)
    {
        double myFreq = value(0).toDouble();
        if(freq != myFreq)
        {
            setValue(0, freq);
            sync();
            emit freqChanged(freq);
        }
    }
    void setDuty(const QVariant& duty)
    {
        double myDuty = value(1).toDouble();
        if(duty != myDuty)
        {
            setValue(1, duty);
            sync();
            emit dutyChanged(duty);
        }
    }
signals:
    void dutyChanged(QVariant value);
    void freqChanged(QVariant value);
};


#endif // PROPERTY_CLASS_H
