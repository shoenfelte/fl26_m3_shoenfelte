#include "aiws/gui/search_widget.hpp"

#include "aiws/gui/search_result_model.hpp"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTableView>
#include <QVBoxLayout>

#include <utility>

namespace aiws::gui {

namespace {
constexpr int kDefaultK = 3;
constexpr int kMinK = 1;
constexpr int kMaxK = 100;
}  // namespace

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    auto* query_label = new QLabel("<b>Query:</b>", this);
    query_edit_ = new QLineEdit(this);
    query_edit_->setPlaceholderText("Search the current corpus");

    auto* k_label = new QLabel("Results:", this);
    result_count_spin_ = new QSpinBox(this);
    result_count_spin_->setRange(kMinK, kMaxK);
    result_count_spin_->setValue(kDefaultK);

    search_button_ = new QPushButton("Search", this);
    search_button_->setEnabled(false);

    result_model_ = new SearchResultModel(this);
    results_view_ = new QTableView(this);
    results_view_->setModel(result_model_);
    results_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    results_view_->verticalHeader()->setVisible(false);
    results_view_->horizontalHeader()->setStretchLastSection(true);
    results_view_->setTextElideMode(Qt::ElideRight);
    results_view_->setWordWrap(false);
    results_view_->setColumnWidth(0, 50);
    results_view_->setColumnWidth(1, 160);
    results_view_->setColumnWidth(2, 55);
    results_view_->setColumnWidth(3, 70);

    auto* selected_label = new QLabel("<b>Selected Result:</b>", this);
    view_source_button_ = new QPushButton("View Source", this);
    view_source_button_->setEnabled(false);

    selected_result_text_ = new QPlainTextEdit(this);
    selected_result_text_->setReadOnly(true);
    selected_result_text_->setPlaceholderText("Select a result to inspect its details.");

    auto* query_row = new QHBoxLayout();
    query_row->addWidget(query_label);
    query_row->addWidget(query_edit_, 1);
    query_row->addWidget(k_label);
    query_row->addWidget(result_count_spin_);
    query_row->addWidget(search_button_);

    auto* selected_row = new QHBoxLayout();
    selected_row->addWidget(selected_label);
    selected_row->addStretch(1);
    selected_row->addWidget(view_source_button_);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addLayout(query_row);
    layout->addWidget(results_view_, 3);
    layout->addLayout(selected_row);
    layout->addWidget(selected_result_text_, 2);

    connect(search_button_, &QPushButton::clicked,
            this, &SearchWidget::request_search);
    connect(query_edit_, &QLineEdit::returnPressed,
            this, &SearchWidget::request_search);
    connect(query_edit_, &QLineEdit::textChanged,
            this, &SearchWidget::query_changed);
    connect(view_source_button_, &QPushButton::clicked,
            this, &SearchWidget::request_source_document);
    connect(results_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SearchWidget::show_selected_result);
}

QString SearchWidget::query() const {
    return query_edit_->text();
}

int SearchWidget::result_count() const {
    return result_count_spin_->value();
}

bool SearchWidget::has_query() const {
    return !query_edit_->text().trimmed().isEmpty();
}

void SearchWidget::set_search_enabled(bool enabled) {
    search_button_->setEnabled(enabled);
}

void SearchWidget::set_results(std::vector<SearchResult> results) {
    clear_results();  // Drops old selection/details first.
    result_model_->set_results(std::move(results));
}

void SearchWidget::clear_results() {
    results_view_->clearSelection();
    result_model_->clear();
    selected_result_text_->clear();
    view_source_button_->setEnabled(false);
    emit selected_chunk_changed(QString());
}

void SearchWidget::reset() {
    clear_results();
    query_edit_->clear();  // Emits query_changed(""); MainWindow refreshes states.
    result_count_spin_->setValue(kDefaultK);
    search_button_->setEnabled(false);
}

void SearchWidget::request_search() {
    // return key can fire while search is disabled
    if (!search_button_->isEnabled() || !has_query()) {
        return;
    }
    emit search_requested(query_edit_->text(), result_count_spin_->value());
}

void SearchWidget::show_selected_result() {
    const QModelIndexList rows = results_view_->selectionModel()->selectedRows();
    const SearchResult* result =
        rows.isEmpty() ? nullptr : result_model_->result_at(rows.first().row());

    if (result == nullptr) {
        selected_result_text_->clear();
        view_source_button_->setEnabled(false);
        emit selected_chunk_changed(QString());
        return;
    }

    const QString details =
        QString("Document: %1\nChunk: %2\nChunk ID: %3\nScore: %4\nMatched terms: %5\n\nText:\n%6")
            .arg(QString::fromStdString(result->document_id))
            .arg(static_cast<qulonglong>(result->chunk_sequence))
            .arg(QString::fromStdString(result->chunk_id))
            .arg(result->score)
            .arg(static_cast<qulonglong>(result->matched_terms))
            .arg(QString::fromStdString(result->text));
    selected_result_text_->setPlainText(details);
    view_source_button_->setEnabled(true);
    emit selected_chunk_changed(QString::fromStdString(result->chunk_id));
}

void SearchWidget::request_source_document() {
    const QModelIndexList rows = results_view_->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return;
    }
    const SearchResult* result = result_model_->result_at(rows.first().row());
    if (result == nullptr) {
        return;
    }
    emit source_document_requested(QString::fromStdString(result->document_id));
}

}  // namespace aiws::gui
