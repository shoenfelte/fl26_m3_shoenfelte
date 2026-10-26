#include "aiws/gui/workspace_widget.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace aiws::gui {

WorkspaceWidget::WorkspaceWidget(QWidget* parent) : QWidget(parent) {
    // LEFT: WORKSPACE
    auto* workspace_label = new QLabel("<b>Workspace</b>", this);
    add_document_button_ = new QPushButton("Add Document", this);
    build_corpus_button_ = new QPushButton("Build Corpus", this);
    build_corpus_button_->setEnabled(false);

    document_list_ = new QListWidget(this);
    document_list_->setSelectionMode(QAbstractItemView::SingleSelection);

    corpus_status_label_ = new QLabel(this);
    set_corpus_status("Not built");

    auto* header_row = new QHBoxLayout();
    header_row->addWidget(workspace_label);
    header_row->addWidget(add_document_button_);
    header_row->addWidget(build_corpus_button_);
    header_row->addStretch(1);

    auto* left = new QVBoxLayout();
    left->addLayout(header_row);
    left->addWidget(document_list_, 1);
    left->addWidget(corpus_status_label_);

    // RIGHT: SELECTED DOCUMENT
    document_title_label_ = new QLabel(this);
    document_title_label_->setTextFormat(Qt::PlainText);
    document_text_ = new QPlainTextEdit(this);
    document_text_->setReadOnly(true);
    document_text_->setPlaceholderText("Select a document to view its contents.");
    clear_selected_document();

    auto* right = new QVBoxLayout();
    right->addWidget(document_title_label_);
    right->addWidget(document_text_, 1);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addLayout(left, 1);
    layout->addLayout(right, 1);

    connect(add_document_button_, &QPushButton::clicked,
            this, &WorkspaceWidget::add_document_requested);
    connect(build_corpus_button_, &QPushButton::clicked,
            this, &WorkspaceWidget::build_corpus_requested);
    connect(document_list_, &QListWidget::currentRowChanged,
            this, &WorkspaceWidget::document_selected);
}

void WorkspaceWidget::add_document(const QString& title) {
    document_list_->addItem(title);  // no change to selection
}

void WorkspaceWidget::select_document(int row) {
    if (row >= 0 && row < document_list_->count()) {
        document_list_->setCurrentRow(row);
    }
}

void WorkspaceWidget::show_selected_document(const QString& title,
                                              const QString& text) {
    document_title_label_->setText("Selected Document - " + title);
    document_text_->setPlainText(text);
}

void WorkspaceWidget::clear_selected_document() {
    document_title_label_->setText("Selected Document -");
    document_text_->clear();
}

void WorkspaceWidget::set_corpus_status(const QString& text) {
    corpus_status_label_->setText("Corpus: <b>" + text.toHtmlEscaped() + "</b>");
}

void WorkspaceWidget::set_build_corpus_enabled(bool enabled) {
    build_corpus_button_->setEnabled(enabled);
}

void WorkspaceWidget::reset() {
    document_list_->clear();
    clear_selected_document();
    set_corpus_status("Not built");
    build_corpus_button_->setEnabled(false);
}

}  // namespace aiws::gui
