#pragma once

#include "ChatMessage.h"
#include <QAbstractListModel>
#include <QList>

/**
 * @brief チャット表示用の QAbstractListModel 派生モデル
 *
 * 1 行 = 1 メッセージ。各行はユーザー発話 / アシスタント応答 / エラーの
 * いずれかを表す。役割と本文を分離して保持し、表示時にだけ
 * "You: " / "AI: " のプレフィックスを付加する。
 *
 * 旧実装（QStringListModel + 文字列プレフィックスでの role 表現）が
 * 抱えていた "本文側に同じプレフィックスが現れると履歴が壊れる" 問題を
 * 構造的に解消する。API 送信用の ChatHistory 取得もモデル自身が担う。
 */
class ChatListModel : public QAbstractListModel {
    Q_OBJECT

public:
    /// 各行が表すメッセージの種別
    enum class ItemKind {
        User,
        Assistant,
        Error,
    };
    Q_ENUM(ItemKind)

    /// data() / setData() で使うカスタムロール
    enum Roles {
        KindRole = Qt::UserRole + 1, ///< ItemKind を返す
        ContentRole,                 ///< 本文（プレフィックス無し）を返す
    };

    explicit ChatListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;

    void appendUserMessage(const QString &content);
    void appendAssistantMessage(const QString &content);
    void appendErrorMessage(const QString &content);

    ItemKind kindAt(int row) const;
    QString contentAt(int row) const;
    bool isAssistantRow(int row) const;

    /// API 送信用に User/Assistant の行のみを抽出した履歴を返す
    ChatHistory toChatHistory() const;

private:
    struct Item {
        ItemKind kind;
        QString content;
    };

    void appendItem(ItemKind kind, const QString &content);

    QList<Item> m_items;
};
