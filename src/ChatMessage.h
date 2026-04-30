#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

/**
 * @brief LLM とやり取りする 1 メッセージ分のデータ
 *
 * LM Studio / OpenAI 互換 API のチャット完了 (chat completion)
 * リクエストにおける 1 件のメッセージを表現するインターフェース型。
 * UI 側の表示形式とは独立に、API 通信層に渡すための中立な構造として用いる。
 *
 * "system" ロールはプロファイルからクライアント側で別途付与されるため、
 * ここでは User / Assistant のみを表す。
 */
struct ChatMessage {
    enum class Role {
        User,
        Assistant,
    };

    Role role = Role::User;
    QString content;
};

/// チャット完了リクエストに渡す履歴の型
using ChatHistory = QList<ChatMessage>;

Q_DECLARE_METATYPE(ChatMessage)
Q_DECLARE_METATYPE(ChatHistory)
