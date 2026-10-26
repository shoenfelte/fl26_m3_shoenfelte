#include "aiws/gui/term_inspector_widget.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aiws::gui {

namespace {
const QString kNeutral = QStringLiteral("\u2014");  // em dash
}

TermInspectorWidget::TermInspectorWidget(QWidget* parent) : QWidget(parent) {
    auto* title = new QLabel("<b>Term Inspector:</b>", this);

    auto* term_label = new QLabel("Term:", this);
    term_edit_ = new QLineEdit(this);
    term_edit_->setPlaceholderText("Enter a term");
    inspect_button_ = new QPushButton("Inspect", this);
    inspect_button_->setEnabled(false);

    auto* df_label = new QLabel("Document Frequency:", this);
    document_frequency_value_ = new QLabel(kNeutral, this);
    auto* tf_label = new QLabel("Selected Chunk TF:", this);
    chunk_frequency_value_ = new QLabel(kNeutral, this);

    auto* term_row = new QHBoxLayout();
    term_row->addWidget(term_label);
    term_row->addWidget(term_edit_, 1);
    term_row->addWidget(inspect_button_);

    auto* stats_row = new QHBoxLayout();
    stats_row->addWidget(df_label);
    stats_row->addWidget(document_frequency_value_);
    stats_row->addSpacing(24);
    stats_row->addWidget(tf_label);
    stats_row->addWidget(chunk_frequency_value_);
    stats_row->addStretch(1);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(title);
    layout->addLayout(term_row);
    layout->addLayout(stats_row);
    layout->addStretch(1);

    // pressing enter in the term field = same request as clicking inspect
    connect(term_edit_, &QLineEdit::returnPressed,
            this, &TermInspectorWidget::request_inspection);
    connect(inspect_button_, &QPushButton::clicked,
            this, &TermInspectorWidget::request_inspection);
}

void TermInspectorWidget::set_inspection_enabled(bool enabled) {
    inspect_button_->setEnabled(enabled);
}

void TermInspectorWidget::set_selected_chunk(const QString& chunk_id) {
    // only record which chunk would be used
    selected_chunk_id_ = chunk_id;
}

void TermInspectorWidget::set_statistics(std::size_t document_frequency,
                                         bool has_chunk_frequency,
                                         std::size_t chunk_frequency) {
    document_frequency_value_->setText(
        QString::number(static_cast<qulonglong>(document_frequency)));
    chunk_frequency_value_->setText(
        has_chunk_frequency
            ? QString::number(static_cast<qulonglong>(chunk_frequency))
            : kNeutral);
}

void TermInspectorWidget::clear_statistics() {
    document_frequency_value_->setText(kNeutral);
    chunk_frequency_value_->setText(kNeutral);
}

void TermInspectorWidget::reset() {
    term_edit_->clear();
    selected_chunk_id_.clear();
    clear_statistics();
    inspect_button_->setEnabled(false);
}

void TermInspectorWidget::request_inspection() {
    // return key can fire while the button is disabled (stale corpus)
    if (!inspect_button_->isEnabled()) {
        return;
    }
    const QString term = term_edit_->text().trimmed();
    if (term.isEmpty()) {
        return;
    }
    emit term_inspection_requested(term, selected_chunk_id_);
}

}  // namespace aiws::gui
