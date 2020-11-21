#include "tabbar.h"

#include "kiwixapp.h"
#include <QAction>
#include <QTimer>
#include <QWebEnginePage>
#include <QToolButton>
#include <QToolTip>
#include <QCursor>

#define QUITIFNULL(VIEW) if (nullptr==(VIEW)) { return; }
#define QUITIFNOTCURRENT(VIEW) if((VIEW)!=currentWidget()) {return;}
#define CURRENTIFNULL(VIEW) if(nullptr==VIEW) { VIEW = currentWidget();}

TabBar::TabBar(QWidget *parent) :
    QTabBar(parent),
    m_settingsIndex(-1)
{
    QTabBar::setDrawBase(false);
    setTabsClosable(true);
    setElideMode(Qt::ElideNone);
    setDocumentMode(true);
    setFocusPolicy(Qt::NoFocus);
    setIconSize(QSize(30, 30));
    connect(this, &QTabBar::currentChanged, this, &TabBar::onCurrentChanged);
    auto app = KiwixApp::instance();

    setStyleSheet(
                "QTabBar::tab"
                " {"
                "    color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,"
                "    stop: 0.8 #2A2A2A, stop: 1.0 #E1E1E1);"
                "  }"
                );

    connect(app->getAction(KiwixApp::NewTabAction), &QAction::triggered,
            this, [=]() {
                this->createNewTab(true);
                auto topWidget = KiwixApp::instance()->getMainWindow()->getTopWidget();
                topWidget->getSearchBar().setFocus(Qt::MouseFocusReason);
                topWidget->getSearchBar().clear();
                topWidget->getSearchBar().clearSuggestions();
                topWidget->getSearchBar().hideSuggestions();
          });
    connect(app->getAction(KiwixApp::CloseTabAction), &QAction::triggered,
            this, [=]() {
                auto index = this->tabAt(mapFromGlobal(QCursor::pos()));
                if (index <= 0) {
                    return;
                }
                this->closeTab(index);
            });
    connect(app->getAction(KiwixApp::OpenHomePageAction), &QAction::triggered,
            this, [=]() {
                auto current = this->currentWebView();
                QUITIFNULL(current);
                current->setUrl("zim://" + current->zimId() + ".zim/");
            });
    connect(app->getAction(KiwixApp::SettingAction), &QAction::triggered,
            this, [=]() {
                if (KiwixApp::instance()->getSettingsManager()->isSettingsViewdisplayed()) {
                    return;
                }
                auto index = currentIndex() + 1;
                m_settingsIndex = index;
                auto view = KiwixApp::instance()->getSettingsManager()->getView();
                mp_stackedWidget->insertWidget(index, view);
                insertTab(index,QIcon(":/icons/settings.svg"), gt("settings"));
                QToolButton *tb = new QToolButton(this);
                tb->setDefaultAction(KiwixApp::instance()->getAction(KiwixApp::CloseTabAction));
                setTabButton(index, QTabBar::RightSide, tb);
                setCurrentIndex(index);
            });
}

void TabBar::setStackedWidget(QStackedWidget *widget) {
    mp_stackedWidget = widget;
    connect(this, &QTabBar::currentChanged,
            widget, &QStackedWidget::setCurrentIndex);
}

void TabBar::setContentManagerView(ContentManagerView* view)
{
    qInfo() << "add widget";
    mp_contentManagerView = view;
    mp_stackedWidget->addWidget(mp_contentManagerView);
    mp_stackedWidget->show();
    addTab(QIcon(":/icons/library-icon.svg"), "");
    setTabButton(0, RightSide, nullptr);
}

void TabBar::setNewTabButton()
{
    QToolButton *tb = new QToolButton();
    tb->setDefaultAction(KiwixApp::instance()->getAction(KiwixApp::NewTabAction));
    tb->setIcon(QIcon(":/icons/new-tab-icon.svg"));
    addTab("");
    setTabEnabled(1, false);
    setTabButton(1, QTabBar::LeftSide, tb);
    tabButton(1, QTabBar::RightSide)->deleteLater();
    setTabButton(1, QTabBar::RightSide, 0);
}

ZimView* TabBar::createNewTab(bool setCurrent)
{
    auto tab = new ZimView(this, this);
    auto index = count() - 1;
    mp_stackedWidget->insertWidget(index, tab);
    insertTab(index, "");
    QToolButton *tb = new QToolButton(this);
    tb->setDefaultAction(KiwixApp::instance()->getAction(KiwixApp::CloseTabAction));
    setTabButton(index, QTabBar::RightSide, tb);
    if (setCurrent) {
        setCurrentIndex(index);
    }
    return tab;
}

void TabBar::openUrl(const QUrl& url, bool newTab)
{
    WebView* webView = currentWebView();
    if (newTab || !webView) {
        webView = createNewTab(true)->getWebView();
    }
    QUITIFNULL(webView);
    webView->setUrl(url);
}

void TabBar::setTitleOf(const QString& title, ZimView* tab)
{
    CURRENTIFNULL(tab);
    if (title.startsWith("zim://")) {
        auto url = QUrl(title);
        setTabText(mp_stackedWidget->indexOf(tab), url.path());
    } else {
        int idx = mp_stackedWidget->indexOf(tab);
        setTabToolTip(idx, title);

        // text_widt = tabsize - icon_size - close_button_size
        // FIXME: close_button_size just measured on the screen
        int text_width = tabSizeHint(idx).width() - iconSize().width() - 32;

        QString cut = fontMetrics().elidedText(title, Qt::ElideRight, text_width);

        // strip ... from the end (this three dots are one char)
        if (cut.size() < title.size())
            cut = cut.mid(0, cut.size() - 1);
        setTabText(idx, cut);
    }
}

void TabBar::setIconOf(const QIcon &icon, ZimView *tab)
{
    CURRENTIFNULL(tab);
    setTabIcon(mp_stackedWidget->indexOf(tab), icon);
}

QString TabBar::currentZimId()
{
    if (!currentWidget())
        return "";
    return currentWebView()->zimId();
}

QString TabBar::currentArticleUrl()
{
    if(!currentWidget())
        return "";
    return currentWebView()->url().path();
}

QString TabBar::currentArticleTitle()
{
    if(!currentWidget())
        return "";
    return currentWebView()->title();
}

QSize TabBar::tabSizeHint(int index) const {
    if (index)
        return QSize(205, 40);
    return QSize(40, 40);
}

void TabBar::openFindInPageBar()
{
    currentWidget()->openFindInPageBar();    
}

void TabBar::triggerWebPageAction(QWebEnginePage::WebAction action, ZimView *widget)
{
    CURRENTIFNULL(widget);
    QUITIFNULL(widget);
    widget->getWebView()->triggerPageAction(action);
    widget->getWebView()->setFocus();
}

void TabBar::closeTab(int index)
{
    if (index == 0 || index == this->count() - 1)
        return;
    if (index == m_settingsIndex) {
        m_settingsIndex = -1;
    }
    if (index < m_settingsIndex) {
        m_settingsIndex--;
    }
    setSelectionBehaviorOnRemove(index);
    auto webview = widget(index);
    mp_stackedWidget->removeWidget(webview);
    webview->setParent(nullptr);
    removeTab(index);
    webview->close();
    delete webview;
}

void TabBar::setSelectionBehaviorOnRemove(int index)
{
    if (index == count() - 2) {
        setCurrentIndex(index - 1);
    } else {
        setCurrentIndex(index + 1);
    }
}

void TabBar::onCurrentChanged(int index)
{
    if (index == -1)
        return;
    if (index == m_settingsIndex) {
        emit webActionEnabledChanged(QWebEnginePage::Back, false);
        emit webActionEnabledChanged(QWebEnginePage::Forward, false);
        emit libraryPageDisplayed(false);
        KiwixApp::instance()->setSideBar(KiwixApp::NONE);
        QTimer::singleShot(0, [=](){emit currentTitleChanged("");});
    } else if (index) {
        auto view = widget(index)->getWebView();
        emit webActionEnabledChanged(QWebEnginePage::Back, view->isWebActionEnabled(QWebEnginePage::Back));
        emit webActionEnabledChanged(QWebEnginePage::Forward, view->isWebActionEnabled(QWebEnginePage::Forward));
        emit libraryPageDisplayed(false);
        if (KiwixApp::instance()->getSideType() == KiwixApp::CONTENTMANAGER_BAR) {
            KiwixApp::instance()->setSideBar(KiwixApp::NONE);
        }
        QTimer::singleShot(0, [=](){emit currentTitleChanged(view->title());});
    } else {
        emit webActionEnabledChanged(QWebEnginePage::Back, false);
        emit webActionEnabledChanged(QWebEnginePage::Forward, false);
        emit libraryPageDisplayed(true);
        KiwixApp::instance()->setSideBar(KiwixApp::CONTENTMANAGER_BAR);
        QTimer::singleShot(0, [=](){emit currentTitleChanged("");});
    }
}

void TabBar::fullScreenRequested(QWebEngineFullScreenRequest request)
{
    if (request.toggleOn()) {
        if (m_fullScreenWindow)
            return;
        request.accept();
        m_fullScreenWindow.reset(new FullScreenWindow(this->currentWebView()));
    } else {
        if (!m_fullScreenWindow)
            return;
        request.accept();
        m_fullScreenWindow.reset();
    }
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        closeTab(this->tabAt(event->pos()));
    } else {
       QTabBar::mousePressEvent(event);
    }
}
