#pragma once

#include <QString>

struct SystemPromptProfile {
    QString id;
    QString name;
    QString prompt;
    double temperature = 0.7;
    int maxTokens = 2048;

    bool operator==(const SystemPromptProfile &other) const {
        return id == other.id;
    }

    bool operator!=(const SystemPromptProfile &other) const {
        return id != other.id;
    }

    QString displayName() const { return name; }

    static SystemPromptProfile
    createDefault(const QString &id, const QString &name,
                  const QString &prompt = "", double temperature = 0.7,
                  int maxTokens = 2048) {
        SystemPromptProfile p;
        p.id = id;
        p.name = name;
        p.prompt = prompt;
        p.temperature = temperature;
        p.maxTokens = maxTokens;
        return p;
    }
};
