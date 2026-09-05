/*
* Audacity: A Digital Audio Editor
*/

#include "commandpalettemodel.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

#include "settingsindex.h"

#include "settings.h"
#include "translation.h"

#include "log.h"

using namespace au::companion;

namespace {
const muse::Settings::Key PALETTE_FULL_WINDOW("companion", "companion/palette/fullWindow");

//! The documentation the palette indexes. In a development tree this is the
//! repository's own docs directory; in an installed build it is the copy that
//! sits next to the application. The first one that exists wins.
QStringList documentationSearchPaths()
{
    QStringList paths;
#ifdef AU_COMPANION_DOCS_DIR
    paths << QStringLiteral(AU_COMPANION_DOCS_DIR);
#endif
    const QString appDir = QCoreApplication::applicationDirPath();
    paths << appDir + QStringLiteral("/docs")
          << appDir + QStringLiteral("/../share/audacity/docs")
          << appDir + QStringLiteral("/../Resources/docs");
    return paths;
}

QString titleFromMarkdown(const QString& path, const QString& fallback)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return fallback;
    }
    for (int line = 0; line < 20 && !file.atEnd(); ++line) {
        const QString text = QString::fromUtf8(file.readLine()).trimmed();
        if (text.startsWith(QStringLiteral("# "))) {
            file.close();
            return text.mid(2).trimmed();
        }
    }
    file.close();
    return fallback;
}
}

CommandPaletteModel::CommandPaletteModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
    muse::settings()->setDefaultValue(PALETTE_FULL_WINDOW, muse::Val(false));
}

void CommandPaletteModel::registerAsPaletteHost()
{
    dispatcher()->reg(this, "companion-command-palette", this,
                      &CommandPaletteModel::onPaletteActionTriggered);
}

void CommandPaletteModel::onPaletteActionTriggered()
{
    emit openRequested();
}

int CommandPaletteModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_visible.size();
}

QHash<int, QByteArray> CommandPaletteModel::roleNames() const
{
    return {
        { RowTypeRole, "rowType" },
        { TitleRole, "title" },
        { SubtitleRole, "subtitle" },
        { SectionRole, "section" },
        { ShortcutRole, "shortcut" },
        { IconRole, "icon" },
        { EnabledRole, "rowEnabled" },
        { CheckableRole, "checkable" },
        { CheckedRole, "checked" },
        { ControlTypeRole, "controlType" },
        { SettingKeyRole, "settingKey" },
        { SettingValueRole, "settingValue" },
        { OptionsRole, "options" },
        { OptionLabelsRole, "optionLabels" },
        { MinimumRole, "minimum" },
        { MaximumRole, "maximum" },
        { StepRole, "step" },
        { ActionCodeRole, "actionCode" },
        { PageIdRole, "pageId" },
        { TargetRole, "target" },
        { PayloadRole, "payload" }
    };
}

QVariant CommandPaletteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return QVariant();
    }
    const Row& row = m_all.at(m_visible.at(index.row()));

    switch (role) {
    case RowTypeRole: return row.type;
    case TitleRole: return row.title;
    case SubtitleRole: return row.subtitle;
    case SectionRole: return row.section;
    case ShortcutRole: return row.shortcut;
    case IconRole: return row.icon;
    case EnabledRole: return row.enabled;
    case CheckableRole: return row.checkable;
    case CheckedRole: return row.checked;
    case ControlTypeRole: return row.controlType;
    case SettingKeyRole: return row.settingKey;
    case SettingValueRole: return row.settingKey.isEmpty() ? QVariant() : settingValue(row.settingKey);
    case OptionsRole: return row.options;
    case OptionLabelsRole: return row.optionLabels;
    case MinimumRole: return row.minimum;
    case MaximumRole: return row.maximum;
    case StepRole: return row.step;
    case ActionCodeRole: return row.actionCode;
    case PageIdRole: return row.pageId;
    case TargetRole: return row.target;
    case PayloadRole: return row.payload;
    default: break;
    }
    return QVariant();
}

QString CommandPaletteModel::filter() const
{
    return m_filter;
}

void CommandPaletteModel::setFilter(const QString& filter)
{
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    applyFilter();
    emit filterChanged();
}

bool CommandPaletteModel::useRegex() const
{
    return m_useRegex;
}

void CommandPaletteModel::setUseRegex(bool value)
{
    if (m_useRegex == value) {
        return;
    }
    m_useRegex = value;
    applyFilter();
    emit useRegexChanged();
    emit filterChanged();
}

bool CommandPaletteModel::filterValid() const
{
    return m_filterValid;
}

QString CommandPaletteModel::filterError() const
{
    return m_filterError;
}

bool CommandPaletteModel::fullWindow() const
{
    return muse::settings()->value(PALETTE_FULL_WINDOW).toBool();
}

void CommandPaletteModel::setFullWindow(bool value)
{
    if (fullWindow() == value) {
        return;
    }
    muse::settings()->setSharedValue(PALETTE_FULL_WINDOW, muse::Val(value));
    emit fullWindowChanged();
}

QString CommandPaletteModel::rowHaystack(const Row& row)
{
    return (row.title + u' ' + row.subtitle + u' ' + row.section + u' '
            + row.shortcut + u' ' + row.actionCode + u' ' + row.settingKey).toLower();
}

void CommandPaletteModel::reload()
{
    beginResetModel();

    m_all.clear();
    buildActions();
    buildPreferences();
    buildAppearance();
    buildDocumentation();

    for (const QVariant& value : std::as_const(m_contextRows)) {
        const QVariantMap map = value.toMap();
        Row row;
        row.type = QStringLiteral("context");
        row.title = map.value(QStringLiteral("title")).toString();
        row.subtitle = map.value(QStringLiteral("subtitle")).toString();
        row.section = map.value(QStringLiteral("section"), QStringLiteral("Window")).toString();
        row.actionCode = map.value(QStringLiteral("actionCode")).toString();
        row.payload = map.value(QStringLiteral("payload")).toMap();
        row.haystack = rowHaystack(row);
        m_all.append(row);
    }

    for (Row& row : m_all) {
        row.haystack = rowHaystack(row);
    }

    endResetModel();

    applyFilter();
    emit countChanged();
}

void CommandPaletteModel::buildActions()
{
    const std::vector<muse::ui::UiAction> actions = actionsRegister()->actionList();

    for (const muse::ui::UiAction& action : actions) {
        if (action.code.empty()) {
            continue;
        }

        Row row;
        row.type = QStringLiteral("action");
        row.actionCode = QString::fromStdString(action.code);
        row.title = action.title.qTranslatedWithoutMnemonic();
        if (row.title.isEmpty()) {
            row.title = row.actionCode;
        }
        row.subtitle = action.description.qTranslated();
        row.icon = static_cast<int>(action.iconCode);
        row.checkable = action.checkable == muse::ui::Checkable::Yes;

        // The section is the first path element of the action code, which is
        // how the muse modules group their actions, with a readable fallback.
        const QString code = row.actionCode;
        const int slash = code.indexOf(u'/');
        QString section = slash > 0 ? code.left(slash) : QStringLiteral("Commands");
        section.remove(QStringLiteral("action:"));
        section.remove(QStringLiteral("//"));
        if (section.isEmpty()) {
            section = QStringLiteral("Commands");
        }
        section[0] = section.at(0).toUpper();
        row.section = section;

        const muse::ui::UiActionState state = actionsRegister()->actionState(action.code);
        row.enabled = state.enabled;
        row.checked = state.checked;

#ifdef MUSE_MODULE_SHORTCUTS
        const muse::shortcuts::Shortcut& shortcut = shortcutsRegister()->shortcut(action.code);
        if (!shortcut.sequences.empty()) {
            row.shortcut = QString::fromStdString(shortcut.sequencesAsString());
        }
#endif

        m_all.append(row);
    }
}

void CommandPaletteModel::buildPreferences()
{
    const SettingsIndex& index = SettingsIndex::instance();
    if (index.isEmpty()) {
        LOGW() << "the command palette settings index is empty; preferences will not be indexed";
        return;
    }

    for (const QVariant& value : index.pages()) {
        const QVariantMap page = value.toMap();
        Row row;
        row.type = QStringLiteral("page");
        row.title = page.value(QStringLiteral("title")).toString();
        row.subtitle = muse::qtrc("companion", "Open this preferences page");
        row.section = page.value(QStringLiteral("section"), QStringLiteral("Preferences")).toString();
        row.pageId = page.value(QStringLiteral("id")).toString();
        m_all.append(row);
    }

    for (const QVariant& value : index.settings()) {
        const QVariantMap setting = value.toMap();
        Row row;
        row.type = QStringLiteral("setting");
        row.title = setting.value(QStringLiteral("label")).toString();
        row.section = QStringLiteral("Preferences: ") + setting.value(QStringLiteral("group")).toString();
        row.subtitle = setting.value(QStringLiteral("keywords")).toString();
        row.controlType = setting.value(QStringLiteral("control")).toString();
        row.settingKey = setting.value(QStringLiteral("key")).toString();
        row.options = setting.value(QStringLiteral("options")).toList();
        row.optionLabels = setting.value(QStringLiteral("optionLabels")).toList();
        row.minimum = setting.value(QStringLiteral("min"), 0.0).toDouble();
        row.maximum = setting.value(QStringLiteral("max"), 1.0).toDouble();
        row.step = setting.value(QStringLiteral("step"), 1.0).toDouble();
        row.pageId = setting.value(QStringLiteral("page")).toString();
        row.target = setting.value(QStringLiteral("target")).toString();
        m_all.append(row);
    }
}

void CommandPaletteModel::buildAppearance()
{
    const SettingsIndex& index = SettingsIndex::instance();

    for (const QVariant& value : index.appearance()) {
        const QVariantMap control = value.toMap();
        Row row;
        row.type = QStringLiteral("appearance");
        row.title = control.value(QStringLiteral("label")).toString();
        row.section = QStringLiteral("Appearance");
        row.controlType = control.value(QStringLiteral("control")).toString();
        row.settingKey = control.value(QStringLiteral("key")).toString();
        row.options = control.value(QStringLiteral("options")).toList();
        row.optionLabels = control.value(QStringLiteral("optionLabels")).toList();
        row.minimum = control.value(QStringLiteral("min"), 0.0).toDouble();
        row.maximum = control.value(QStringLiteral("max"), 1.0).toDouble();
        row.step = control.value(QStringLiteral("step"), 1.0).toDouble();
        row.pageId = control.value(QStringLiteral("page")).toString();
        row.target = control.value(QStringLiteral("target")).toString();
        row.subtitle = row.settingKey;
        m_all.append(row);
    }
}

void CommandPaletteModel::buildDocumentation()
{
    m_documentationRoot.clear();

    QString root;
    for (const QString& candidate : documentationSearchPaths()) {
        if (QFileInfo::exists(candidate)) {
            root = QDir(candidate).absolutePath();
            break;
        }
    }
    if (root.isEmpty()) {
        return;
    }
    m_documentationRoot = root;

    QDirIterator it(root, QStringList { QStringLiteral("*.md") }, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString relative = QDir(root).relativeFilePath(path);
        // The site's own copies duplicate docs/features, so only one of the two
        // is indexed.
        if (relative.startsWith(QStringLiteral("site/docs/"))) {
            continue;
        }

        Row row;
        row.type = QStringLiteral("doc");
        row.title = titleFromMarkdown(path, QFileInfo(path).completeBaseName());
        row.subtitle = relative;
        row.section = QStringLiteral("Documentation");
        row.payload.insert(QStringLiteral("path"), path);
        m_all.append(row);
    }
}

void CommandPaletteModel::setContextRows(const QVariantList& rows)
{
    m_contextRows = rows;
    reload();
}

void CommandPaletteModel::applyFilter()
{
    beginResetModel();

    m_visible.clear();
    m_filterValid = true;
    m_filterError.clear();

    const QString needle = m_filter.trimmed();
    if (needle.isEmpty()) {
        m_visible.reserve(m_all.size());
        for (int i = 0; i < m_all.size(); ++i) {
            m_visible.append(i);
        }
        endResetModel();
        emit countChanged();
        return;
    }

    if (m_useRegex) {
        QRegularExpression expression(needle, QRegularExpression::CaseInsensitiveOption);
        if (!expression.isValid()) {
            m_filterValid = false;
            m_filterError = expression.errorString();
            endResetModel();
            emit countChanged();
            return;
        }
        expression.optimize();
        for (int i = 0; i < m_all.size(); ++i) {
            if (expression.match(m_all.at(i).haystack).hasMatch()) {
                m_visible.append(i);
            }
        }
    } else {
        // Plain text: every whitespace separated word must appear somewhere in
        // the row, in any order, which is what makes "pref dens" find the
        // density slider.
        const QStringList words = needle.toLower().split(QRegularExpression(QStringLiteral("\\s+")),
                                                         Qt::SkipEmptyParts);
        for (int i = 0; i < m_all.size(); ++i) {
            const QString& haystack = m_all.at(i).haystack;
            bool all = true;
            for (const QString& word : words) {
                if (!haystack.contains(word)) {
                    all = false;
                    break;
                }
            }
            if (all) {
                m_visible.append(i);
            }
        }
    }

    endResetModel();
    emit countChanged();
}

void CommandPaletteModel::activate(int row)
{
    if (row < 0 || row >= m_visible.size()) {
        return;
    }
    const Row& item = m_all.at(m_visible.at(row));

    if (item.type == QStringLiteral("action")) {
        if (!item.enabled) {
            return;
        }
        dispatcher()->dispatch(item.actionCode.toStdString());
        emit activated();
        return;
    }

    if (item.type == QStringLiteral("doc")) {
        emit teleportToDocument(item.payload.value(QStringLiteral("path")).toString());
        emit activated();
        return;
    }

    if (item.type == QStringLiteral("context")) {
        if (!item.actionCode.isEmpty()) {
            dispatcher()->dispatch(item.actionCode.toStdString());
        }
        emit teleportToContext(item.payload);
        emit activated();
        return;
    }

    // A page, a preference or an appearance control: teleport to the surface
    // that owns it.
    if (!item.pageId.isEmpty()) {
        emit teleportToPreferences(item.pageId, item.target);
        emit activated();
    }
}

QVariant CommandPaletteModel::settingValue(const QString& key) const
{
    if (key.isEmpty()) {
        return QVariant();
    }
    const std::string k = key.toStdString();
    const muse::Settings::Key settingKey(k.substr(0, k.find('/')), k);
    return muse::settings()->value(settingKey).toQVariant();
}

void CommandPaletteModel::setSettingValue(const QString& key, const QVariant& value)
{
    if (key.isEmpty()) {
        return;
    }
    const std::string k = key.toStdString();
    const muse::Settings::Key settingKey(k.substr(0, k.find('/')), k);
    muse::settings()->setSharedValue(settingKey, muse::Val::fromQVariant(value));

    // The row's own value role has to refresh so that the inline control shows
    // what the setting now holds.
    for (int i = 0; i < m_visible.size(); ++i) {
        if (m_all.at(m_visible.at(i)).settingKey == key) {
            const QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx, { SettingValueRole });
        }
    }
}

QStringList CommandPaletteModel::indexedSettingKeys() const
{
    QStringList keys;
    for (const Row& row : m_all) {
        if (!row.settingKey.isEmpty() && !keys.contains(row.settingKey)) {
            keys.append(row.settingKey);
        }
    }
    return keys;
}

QStringList CommandPaletteModel::indexedTargets() const
{
    return SettingsIndex::instance().targets();
}

QString CommandPaletteModel::documentationRoot() const
{
    return m_documentationRoot;
}
