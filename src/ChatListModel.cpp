#include "ChatListModel.h"

namespace {

constexpr QLatin1String kUserPrefix("You: ");
constexpr QLatin1String kAssistantPrefix("AI: ");

QLatin1String prefixForKind(ChatListModel::ItemKind kind) {
    switch (kind) {
    case ChatListModel::ItemKind::User:
        return kUserPrefix;
    case ChatListModel::ItemKind::Assistant:
        return kAssistantPrefix;
    case ChatListModel::ItemKind::Error:
        return QLatin1String("");
    }
    Q_UNREACHABLE();
}

} // namespace

ChatListModel::ChatListModel(QObject *parent) : QAbstractListModel(parent) {}

int ChatListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Item &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        // 既存ビューの見た目を維持するため、プレフィックスを付与して返す
        return prefixForKind(item.kind) + item.content;
    case Qt::EditRole:
    case ContentRole:
        return item.content;
    case KindRole:
        return QVariant::fromValue(item.kind);
    default:
        return {};
    }
}

bool ChatListModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return false;
    }
    if (role != Qt::EditRole && role != ContentRole) {
        return false;
    }
    const QString newContent = value.toString();
    Item &item = m_items[index.row()];
    if (item.content == newContent) {
        return true;
    }
    item.content = newContent;
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, ContentRole});
    return true;
}

Qt::ItemFlags ChatListModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

bool ChatListModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (parent.isValid() || count <= 0 || row < 0 ||
        row + count > m_items.size()) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    m_items.erase(m_items.begin() + row, m_items.begin() + row + count);
    endRemoveRows();
    return true;
}

QHash<int, QByteArray> ChatListModel::roleNames() const {
    QHash<int, QByteArray> names = QAbstractListModel::roleNames();
    names.insert(KindRole, "kind");
    names.insert(ContentRole, "content");
    return names;
}

void ChatListModel::appendUserMessage(const QString &content) {
    appendItem(ItemKind::User, content);
}

void ChatListModel::appendAssistantMessage(const QString &content) {
    appendItem(ItemKind::Assistant, content);
}

void ChatListModel::appendErrorMessage(const QString &content) {
    appendItem(ItemKind::Error, content);
}

ChatListModel::ItemKind ChatListModel::kindAt(int row) const {
    Q_ASSERT(row >= 0 && row < m_items.size());
    return m_items.at(row).kind;
}

QString ChatListModel::contentAt(int row) const {
    Q_ASSERT(row >= 0 && row < m_items.size());
    return m_items.at(row).content;
}

bool ChatListModel::isAssistantRow(int row) const {
    if (row < 0 || row >= m_items.size()) {
        return false;
    }
    return m_items.at(row).kind == ItemKind::Assistant;
}

ChatHistory ChatListModel::toChatHistory() const {
    ChatHistory history;
    history.reserve(m_items.size());
    for (const Item &item : m_items) {
        switch (item.kind) {
        case ItemKind::User:
            history.append({ChatMessage::Role::User, item.content});
            break;
        case ItemKind::Assistant:
            history.append({ChatMessage::Role::Assistant, item.content});
            break;
        case ItemKind::Error:
            // エラー行は履歴に含めない
            break;
        }
    }
    return history;
}

void ChatListModel::appendItem(ItemKind kind, const QString &content) {
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append({kind, content});
    endInsertRows();
}
