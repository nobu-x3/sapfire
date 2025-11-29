#include "widgets/asset_delegate.h"
#include <QApplication>
#include <QPainter>
#include "widgets/file_system_model.h"

SQAssetDelegate::SQAssetDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void SQAssetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    // Draw background (selection/hover)
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text = ""; // We'll draw text manually
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    // Get data
    QVariant thumbVar = index.data(SQFileSystemModel::ThumbnailRole);
    QPixmap thumbnail = thumbVar.value<QPixmap>();
    QString name = index.data(Qt::DisplayRole).toString();
    // Layout parameters
    const int padding = 4;
    const int text_height = opt.fontMetrics.height();
    const int total_height = option.rect.height();
    const int image_height = total_height - text_height - 3 * padding;
    // Draw thumbnail (centered)
    if (!thumbnail.isNull()) {
        QRect img_rect = option.rect;
        img_rect.setTop(img_rect.top() + padding);
        img_rect.setHeight(image_height);
        QSize scaled = thumbnail.size().scaled(img_rect.size(), Qt::KeepAspectRatio);
        QRect target_rect(QPoint(0, 0), scaled);
        target_rect.moveCenter(img_rect.center());
        painter->drawPixmap(target_rect, thumbnail);
    }
    // Draw name (centered at bottom)
    QRect text_rect = option.rect;
    text_rect.setTop(text_rect.top() + image_height + 2 * padding);
    text_rect.setHeight(text_height);
    QString elided_name = opt.fontMetrics.elidedText(name, Qt::ElideRight, text_rect.width());
    painter->drawText(text_rect, Qt::AlignCenter, elided_name);
    painter->restore();
}

QSize SQAssetDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    // Fixed size for items
    return QSize(140, 160); // Width x Height
}
