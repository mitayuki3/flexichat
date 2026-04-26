#pragma once

#include <QString>
#include <QMetaType>

struct SystemPromptProfile {
    QString id;
    QString name;
    QString icon;
    QString prompt;
    double temperature = 0.7;
    int maxTokens = 2048;

    bool operator==(const SystemPromptProfile &other) const {
        return id == other.id;
    }

    bool operator!=(const SystemPromptProfile &other) const {
        return id != other.id;
    }

    QString displayName() const {
        if (icon.isEmpty()) {
            return name;
        }
        return icon + " " + name;
    }

    static SystemPromptProfile
    createDefault(const QString &id, const QString &name, const QString &icon,
                  const QString &prompt = "", double temperature = 0.7,
                  int maxTokens = 2048) {
        SystemPromptProfile p;
        p.id = id;
        p.name = name;
        p.icon = icon;
        p.prompt = prompt;
        p.temperature = temperature;
        p.maxTokens = maxTokens;
        return p;
    }
};

// QDataStream serialization support
inline QDataStream &operator<<(QDataStream &out, const SystemPromptProfile &profile) {
    out << profile.id << profile.name << profile.icon << profile.prompt 
        << profile.temperature << profile.maxTokens;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, SystemPromptProfile &profile) {
    in >> profile.id >> profile.name >> profile.icon >> profile.prompt 
       >> profile.temperature >> profile.maxTokens;
    return in;
}

Q_DECLARE_METATYPE(SystemPromptProfile)
