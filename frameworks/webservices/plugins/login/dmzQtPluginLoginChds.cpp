#include <dmzQtConfigRead.h>
#include <dmzQtConfigWrite.h>
#include <dmzQtModuleMainWindow.h>
#include "dmzQtPluginLoginChds.h"
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigWrite.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>
#include <dmzWebServicesConsts.h>
#include <QtGui/QtGui>
#include <QtCore/QCryptographicHash>


dmz::QtPluginLoginChds::QtPluginLoginChds (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _mainWindowModule (0),
      _mainWindowModuleName (),
      _nameHandle (0),
      _passwordHandle (0),
      _targetHandle (0),
      _loggedIn (False),
      _loginDialog (0),
      _pictureGroup (0),
      _colorGroup (0) {

   _init (local);
}


dmz::QtPluginLoginChds::~QtPluginLoginChds () {

   if (_loginDialog) {

      _loginDialog->setParent (0);
      delete _loginDialog;
      _loginDialog = 0;
   }
}


// Plugin Interface
void
dmz::QtPluginLoginChds::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   const QByteArray Key (get_plugin_name ().get_buffer ());
   RuntimeContext *context (get_plugin_runtime_context ());

   if (State == PluginStateInit) {

      Config session (get_session_config (get_plugin_name (), context));

//      if (_loginDialog) {

//         String user = config_to_string ("user.value", session);
//         _ui.userNameLineEdit->setText (user.get_buffer ());
//      }
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

      if (_loginDialog) {

         Config session (get_plugin_name ());
         String user = qPrintable (_ui.userNameLineEdit->text ());
         QByteArray ba = QCryptographicHash::hash (user.get_buffer (), QCryptographicHash::Sha1);
         session.add_config (
            string_to_config ("user", "value", qPrintable(QString(ba.toHex ()))));
         set_session_config (context, session);
      }
   }
}


void
dmz::QtPluginLoginChds::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_mainWindowModule) {

         _mainWindowModule = QtModuleMainWindow::cast (PluginPtr, _mainWindowModuleName);
         if (_mainWindowModule) {

            _create_dialog (_mainWindowModule->get_qt_main_window ());
         }
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_mainWindowModule &&
            (_mainWindowModule == QtModuleMainWindow::cast (PluginPtr))) {

         if (_loginDialog) { _loginDialog->setParent (0); }

         _mainWindowModule = 0;
      }
   }
}


// Message Observer Interface
void
dmz::QtPluginLoginChds::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _loginRequiredMsg) {

      if (_loginDialog) {

         _slot_update_dialog ();
         _loginDialog->open ();
      }
   }
   else if (Type == _loginSuccessMsg) {

      if (InData) {

         String user;
         InData->lookup_string (_nameHandle, 0, user);

         if (user) { _loggedIn = True; }
      }
   }
   else if (Type == _loginFailedMsg) {

      _loggedIn = False;

      if (_loginDialog) { _loginDialog->open (); }
   }
}


void
dmz::QtPluginLoginChds::_slot_dialog_accepted () {

   if (_loginDialog && _user && _picture && _color) {

      Data data;
      data.store_string (_nameHandle, 0, _user);
      data.store_string (_passwordHandle, 0, _picture + _color);

      _loginMsg.send (_targetHandle, &data);
   }
}


void
dmz::QtPluginLoginChds::_slot_dialog_rejected () {


}


// QtPluginLoginChds Interface
void
dmz::QtPluginLoginChds::_slot_update_dialog () {

   Boolean enabled (False);

   QAbstractButton *pictureButton = _pictureGroup->checkedButton ();
   QAbstractButton *colorButton = _colorGroup->checkedButton ();

   if (pictureButton && colorButton) {

      _picture = qPrintable (pictureButton->text ());
      _color = qPrintable (colorButton->text ());

      QString name = _ui.userNameLineEdit->text ();
      QByteArray ba = QCryptographicHash::hash (name.toLocal8Bit (), QCryptographicHash::Sha1);
      _user = qPrintable (QString(ba.toHex ()));
      if (_user) { enabled = True; }
   }

   QPushButton *button = _ui.buttonBox->button (QDialogButtonBox::Ok);
   if (button) { button->setEnabled (enabled); }
}


void
dmz::QtPluginLoginChds::_create_dialog (QWidget *parent) {

   if (!_loginDialog) {

      _loginDialog = new QDialog (parent);
      _ui.setupUi (_loginDialog);

      connect (
         _loginDialog, SIGNAL (accepted ()),
         this, SLOT (_slot_dialog_accepted ()));

      connect (
         _loginDialog, SIGNAL (rejected ()),
         this, SLOT (_slot_dialog_rejected ()));

      connect (
         _ui.userNameLineEdit, SIGNAL (textChanged (const QString &)),
         this, SLOT (_slot_update_dialog ()));

      _init_pictures ();
      _init_colors ();
   }
}


void
dmz::QtPluginLoginChds::_init_pictures () {

   _pictureGroup = new QButtonGroup (_loginDialog);

   connect (
      _pictureGroup, SIGNAL (buttonClicked (QAbstractButton *)),
      this, SLOT (_slot_update_dialog ()));

   QGridLayout *grid = new QGridLayout;

   Config rows;
   _pictureConfig.lookup_all_config ("row", rows);

   Int32 row (0);
   ConfigIterator rowIt;
   Config rowData;

   while (rows.get_next_config (rowIt, rowData)) {

      Int32 col (0);
      ConfigIterator colIt;
      Config colData;

      while (rowData.get_next_config (colIt, colData)) {

         String value = config_to_string ("value", colData);

         String icon;
         icon << ":/chds/" << value << ".png";

         icon = config_to_string ("icon", colData, icon);

         QToolButton *button = new QToolButton (_ui.pictureGroupBox);
         button->setAutoRaise (True);
         button->setText (value.get_buffer ());

         button->setIcon (QIcon (icon.get_buffer ()));
         button->setIconSize (QSize (32, 32));

         button->setCheckable (True);
         button->setAutoExclusive (True);

         _pictureGroup->addButton (button);
         grid->addWidget (button, row, col);

         col++;
      }

      row++;
   }

   QLayout *prevLayout = _ui.pictureGroupBox->layout ();
   if (prevLayout) { delete prevLayout; }

   _ui.pictureGroupBox->setLayout (grid);
}


void
dmz::QtPluginLoginChds::_init_colors () {

   _colorGroup = new QButtonGroup (_loginDialog);

   connect (
      _colorGroup, SIGNAL (buttonClicked (QAbstractButton *)),
      this, SLOT (_slot_update_dialog ()));

   QGridLayout *grid = new QGridLayout;

   Config rows;
   _colorConfig.lookup_all_config ("row", rows);

   Int32 row (0);
   ConfigIterator rowIt;
   Config rowData;

   while (rows.get_next_config (rowIt, rowData)) {

      Int32 col (0);
      ConfigIterator colIt;
      Config colData;

      while (rowData.get_next_config (colIt, colData)) {

         String value = config_to_string ("value", colData);

         QColor color (value.get_buffer ());
         QPixmap pm (32, 32);
         pm.fill (color);

         QToolButton *button = new QToolButton (_ui.colorGroupBox);
         button->setAutoRaise (True);
         button->setText (value.get_buffer ());

         button->setIcon (QIcon (pm));
         button->setIconSize (QSize (32, 32));

         button->setCheckable (True);
         button->setAutoExclusive (True);

         _colorGroup->addButton (button);
         grid->addWidget (button, row, col);

         col++;
      }

      row++;
   }

   QLayout *prevLayout = _ui.colorGroupBox->layout ();
   if (prevLayout) { delete prevLayout; }

   _ui.colorGroupBox->setLayout (grid);
}


void
dmz::QtPluginLoginChds::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   _mainWindowModuleName = config_to_string ("module.main-window.name", local);

   _nameHandle = config_to_named_handle (
      "attribute.name", local, "name", context);

   _passwordHandle = config_to_named_handle (
      "attribute.password", local, "password", context);

   _loginRequiredMsg = config_create_message (
      "message.login-required",
      local,
      WebServicesLoginRequiredMessageName,
      context);

   _loginMsg = config_create_message (
      "message.login",
      local,
      WebServicesLoginMessageName,
      context);

   _loginSuccessMsg = config_create_message (
      "message.login-success",
      local,
      WebServicesLoginSuccessMessageName,
      context);

   _loginFailedMsg = config_create_message (
      "message.login-failed",
      local,
      WebServicesLoginFailedMessageName,
      context);

   subscribe_to_message (_loginRequiredMsg);
   subscribe_to_message (_loginSuccessMsg);
   subscribe_to_message (_loginFailedMsg);

   _targetHandle = config_to_named_handle ("login-target.name", local, context);

   local.lookup_config ("picture-grid", _pictureConfig);
   local.lookup_config ("color-grid", _colorConfig);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzQtPluginLoginChds (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::QtPluginLoginChds (Info, local);
}

};
