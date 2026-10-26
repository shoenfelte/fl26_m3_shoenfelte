#include "aiws/gui/context_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

namespace aiws::gui {

namespace {
constexpr int kBudgetMin = 50;
constexpr int kBudgetMax = 1000;
constexpr int kBudgetStep = 50;
constexpr int kBudgetStart = 300;
}  // namespace

ContextWidget::ContextWidget(QWidget* parent) : QWidget(parent) {
    auto* title = new QLabel("<b>Context:</b>", this);

    auto* budget_label = new QLabel("Token budget:", this);
    context_budget_slider_ = new QSlider(Qt::Horizontal, this);
    context_budget_slider_->setRange(kBudgetMin, kBudgetMax);
    context_budget_slider_->setSingleStep(kBudgetStep);
    context_budget_slider_->setPageStep(kBudgetStep);
    context_budget_slider_->setTickInterval(kBudgetStep);
    context_budget_slider_->setValue(kBudgetStart);

    budget_value_label_ = new QLabel(QString::number(kBudgetStart), this);
    budget_value_label_->setMinimumWidth(40);
    budget_value_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    build_context_button_ = new QPushButton("Build Context", this);
    build_context_button_->setEnabled(false);

    context_text_ = new QPlainTextEdit(this);
    context_text_->setReadOnly(true);
    context_text_->setPlaceholderText("Built context will appear here.");

    auto* controls = new QHBoxLayout();
    controls->addWidget(budget_label);
    controls->addWidget(context_budget_slider_, 1);
    controls->addWidget(budget_value_label_);
    controls->addWidget(build_context_button_);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(title);
    layout->addLayout(controls);
    layout->addWidget(context_text_, 1);

    connect(context_budget_slider_, &QSlider::valueChanged,
            this, &ContextWidget::budget_changed);
    connect(build_context_button_, &QPushButton::clicked,
            this, &ContextWidget::request_context);
}

int ContextWidget::token_budget() const {
    return context_budget_slider_->value();
}

void ContextWidget::set_build_context_enabled(bool enabled) {
    build_context_button_->setEnabled(enabled);
}

void ContextWidget::set_context(const std::vector<ContextItem>& items) {
    QStringList blocks;
    for (const ContextItem& item : items) {
        QString block;
        block += QString("Document: %1\n").arg(QString::fromStdString(item.document_id));
        block += QString("Chunk: %1\n").arg(static_cast<qulonglong>(item.chunk_sequence));
        block += QString("Tokens: %1\n").arg(static_cast<qulonglong>(item.token_count));
        block += QString("Score: %1\n").arg(item.score);
        block += QString("Truncated: %1\n").arg(item.truncated ? "true" : "false");
        block += QString("Text: %1").arg(QString::fromStdString(item.text));
        blocks << block;
    }
    context_text_->setPlainText(blocks.join("\n\n"));
}

void ContextWidget::clear_context() {
    context_text_->clear();
}

void ContextWidget::reset() {
    context_budget_slider_->setValue(kBudgetStart);
    budget_value_label_->setText(QString::number(kBudgetStart));
    clear_context();
    build_context_button_->setEnabled(false);
}

void ContextWidget::request_context() {
    emit build_context_requested(token_budget());
}

void ContextWidget::budget_changed(int value) {
    // snap to 50 token step, then show value (display only)
    const int snapped = ((value + kBudgetStep / 2) / kBudgetStep) * kBudgetStep;
    if (snapped != value) {
        context_budget_slider_->setValue(snapped);  // re-enters with snapped value
        return;
    }
    budget_value_label_->setText(QString::number(value));
}

}  // namespace aiws::gui
