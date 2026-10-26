#include "aiws/gui/main_window.hpp"

#include "aiws/gui/context_widget.hpp"
#include "aiws/gui/resettable.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/workspace_widget.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <array>
#include <exception>
#include <string>
#include <utility>

namespace aiws::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("AI Workspace");
    resize(900, 900);  // initial

    auto* central = new QWidget(this);
    setCentralWidget(central);

    // HEADER
    auto* header = new QFrame(central);
    header->setObjectName("header");
    header->setStyleSheet("#header { background-color: #6b1f2a; }");

    auto* logo = new QLabel("AI", header);
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedSize(40, 40);
    logo->setStyleSheet(
        "background-color: #8c3a46; color: white; font-weight: bold;"
        "border: 2px solid white; border-radius: 20px;");

    auto* title = new QLabel("AI Workspace", header);
    title->setStyleSheet("color: white; font-size: 22px; font-weight: bold;"
                         "background: transparent;");
    auto* subtitle = new QLabel("Document Search and Context Explorer", header);
    subtitle->setStyleSheet("color: #f0dde0; font-style: italic;"
                            "background: transparent;");

    auto* title_column = new QVBoxLayout();
    title_column->setSpacing(0);
    title_column->addWidget(title);
    title_column->addWidget(subtitle);

    auto* header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(14, 10, 14, 10);
    header_layout->addWidget(logo);
    header_layout->addLayout(title_column);
    header_layout->addStretch(1);

    // COMPONENTS
    workspace_widget_ = new WorkspaceWidget(central);
    search_widget_ = new SearchWidget(central);
    term_inspector_widget_ = new TermInspectorWidget(central);
    context_widget_ = new ContextWidget(central);

    auto* line1 = new QFrame(central);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    auto* line2 = new QFrame(central);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    auto* vline = new QFrame(central);
    vline->setFrameShape(QFrame::VLine);
    vline->setFrameShadow(QFrame::Sunken);

    auto* bottom_row = new QHBoxLayout();
    bottom_row->addWidget(term_inspector_widget_, 1);
    bottom_row->addWidget(vline);
    bottom_row->addWidget(context_widget_, 1);

    // STATUS, RESET, EXIT ROW
    auto* status_caption = new QLabel("<b>Current status:</b>", central);
    status_label_ = new QLabel("Ready", central);
    reset_button_ = new QPushButton("Reset", central);
    reset_button_->setStyleSheet(
        "QPushButton { background-color: #d9a441; color: #2b1d00; font-weight: bold;"
        " border: 1px solid #b58529; border-radius: 3px; padding: 5px 24px; }"
        "QPushButton:hover { background-color: #e6b555; }"
        "QPushButton:pressed { background-color: #c8922f; }");
    exit_button_ = new QPushButton("Exit", central);
    exit_button_->setStyleSheet(
        "QPushButton { background-color: #3b6fb6; color: white; font-weight: bold;"
        " border: 1px solid #2d5590; border-radius: 3px; padding: 5px 24px; }"
        "QPushButton:hover { background-color: #4a80cb; }"
        "QPushButton:pressed { background-color: #2d5590; }");

    auto* footer = new QHBoxLayout();
    footer->setContentsMargins(6, 0, 6, 0);
    footer->addWidget(status_caption);
    footer->addWidget(status_label_, 1);
    footer->addWidget(reset_button_);
    footer->addWidget(exit_button_);

    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 6);
    layout->setSpacing(4);
    layout->addWidget(header);
    layout->addWidget(workspace_widget_, 3);
    layout->addWidget(line1);
    layout->addWidget(search_widget_, 4);
    layout->addWidget(line2);
    layout->addLayout(bottom_row, 2);
    layout->addLayout(footer);

   
    connect(workspace_widget_, &WorkspaceWidget::add_document_requested,
            this, &MainWindow::add_document);
    connect(workspace_widget_, &WorkspaceWidget::build_corpus_requested,
            this, &MainWindow::build_corpus);
    connect(workspace_widget_, &WorkspaceWidget::document_selected,
            this, &MainWindow::document_selected);

    connect(search_widget_, &SearchWidget::search_requested,
            this, &MainWindow::search_requested);
    connect(search_widget_, &SearchWidget::query_changed,
            this, &MainWindow::query_changed);
    connect(search_widget_, &SearchWidget::source_document_requested,
            this, &MainWindow::source_document_requested);
    // result selection only tells term nspector which chunk to use for TF
    connect(search_widget_, &SearchWidget::selected_chunk_changed,
            term_inspector_widget_, &TermInspectorWidget::set_selected_chunk);

    connect(term_inspector_widget_, &TermInspectorWidget::term_inspection_requested,
            this, &MainWindow::term_inspection_requested);
    connect(context_widget_, &ContextWidget::build_context_requested,
            this, &MainWindow::build_context_requested);

    connect(reset_button_, &QPushButton::clicked,
            this, &MainWindow::reset_application);
    connect(exit_button_, &QPushButton::clicked,
            this, &MainWindow::exit_application);

    update_action_states();
}

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_ = callback;
}

void MainWindow::add_document() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Add Document", QString(), "Text files (*.txt);;All files (*)");
    if (path.isEmpty()) {
        return;  // no chNGE
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Add Document",
                             "Could not read file:\n" + path);
        return;
    }
    const QByteArray bytes = file.readAll();
    file.close();

    const QFileInfo info(path);
    const QString id = info.absoluteFilePath();
    const QString title = info.fileName();

    workspace_.add_document(
        Document(id.toStdString(), title.toStdString(), bytes.toStdString()));
    workspace_widget_->add_document(title);

    // data changed
    invalidate_corpus();
    update_action_states();
    status_label_->setText("Added document: " + title);
}

void MainWindow::build_corpus() {
    if (workspace_.documents().empty()) {
        return;
    }
    try {
        processing_core_.rebuild(workspace_);
    } catch (const std::exception& e) {
        corpus_current_ = false;
        status_label_->setText(QString("Build failed: ") + e.what());
        update_action_states();
        return;
    }
    corpus_current_ = true;
    clear_processing_outputs();
    term_inspector_widget_->clear_statistics();

    const auto chunks = static_cast<qulonglong>(processing_core_.chunk_count());
    workspace_widget_->set_corpus_status(QString("%1 chunks").arg(chunks));
    status_label_->setText(QString("Corpus built: %1 chunks").arg(chunks));
    update_action_states();
}

void MainWindow::document_selected(int row) {
    const auto& docs = workspace_.documents();
    if (row < 0 || static_cast<std::size_t>(row) >= docs.size()) {
        workspace_widget_->clear_selected_document();
        return;
    }
    const Document& doc = docs[static_cast<std::size_t>(row)];
    workspace_widget_->show_selected_document(
        QString::fromStdString(doc.title()), QString::fromStdString(doc.text()));
}

void MainWindow::search_requested(const QString& query, int k) {
    if (!corpus_current_ || query.trimmed().isEmpty()) {
        return;
    }
    search_widget_->set_results(processing_core_.search(query.toStdString(), k));
    context_widget_->clear_context();
    status_label_->setText("Search complete.");
    update_action_states();
}

void MainWindow::query_changed(const QString& query) {
    (void)query;
    // availability only: never auto-search, never clear existing output
    update_action_states();
}

void MainWindow::build_context_requested(int token_budget) {
    if (!corpus_current_ || !search_widget_->has_query()) {
        return;
    }
    const auto items = processing_core_.build_context(
        search_widget_->query().toStdString(), search_widget_->result_count(),
        static_cast<std::size_t>(token_budget));
    context_widget_->set_context(items);
    status_label_->setText(
        QString("Context built: %1 items.").arg(static_cast<qulonglong>(items.size())));
}

void MainWindow::source_document_requested(const QString& document_id) {
    const std::string id = document_id.toStdString();
    const auto& docs = workspace_.documents();
    for (std::size_t i = 0; i < docs.size(); ++i) {
        if (docs[i].id() == id) {
            workspace_widget_->select_document(static_cast<int>(i));
            document_selected(static_cast<int>(i));  // ensure display even if row unchanged
            status_label_->setText("Showing source document: " +
                                   QString::fromStdString(docs[i].title()));
            return;
        }
    }
    status_label_->setText("Source document not found in workspace.");
}

void MainWindow::term_inspection_requested(const QString& term,
                                           const QString& chunk_id) {
    if (!corpus_current_ || term.trimmed().isEmpty()) {
        return;
    }
    const std::string t = term.toStdString();
    const std::size_t df = processing_core_.document_frequency(t);
    const bool has_chunk = !chunk_id.isEmpty();
    const std::size_t tf =
        has_chunk ? processing_core_.term_frequency(t, chunk_id.toStdString()) : 0;
    term_inspector_widget_->set_statistics(df, has_chunk, tf);
    status_label_->setText("Inspected term: " + term);
}

void MainWindow::reset_application() {
    workspace_.clear();
    corpus_current_ = false;

    // four different component types through one interface
    const std::array<Resettable*, 4> components = {
        workspace_widget_, search_widget_, term_inspector_widget_, context_widget_};
    for (Resettable* component : components) {
        component->reset();
    }

    update_action_states();
    status_label_->setText("Application reset.");
}

void MainWindow::exit_application() {
    if (exit_callback_ != nullptr) {
        exit_callback_(this);
    }
}

void MainWindow::update_action_states() {
    const bool has_documents = !workspace_.documents().empty();
    const bool has_query = search_widget_->has_query();

    workspace_widget_->set_build_corpus_enabled(has_documents);
    search_widget_->set_search_enabled(corpus_current_ && has_query);
    term_inspector_widget_->set_inspection_enabled(corpus_current_);
    context_widget_->set_build_context_enabled(corpus_current_ && has_query);
}

void MainWindow::invalidate_corpus() {
    corpus_current_ = false;
    workspace_widget_->set_corpus_status("Not built");
    clear_processing_outputs();
    term_inspector_widget_->clear_statistics();
    update_action_states();
}

void MainWindow::clear_processing_outputs() {
    search_widget_->clear_results();  // table, selection, details, view source
    context_widget_->clear_context();
}

}  // namespace aiws::gui
