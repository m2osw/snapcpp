// Snap Websites Server -- JavaScript plugin to run scripts on the server side
// Copyright (C) 2012  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// This language is based on the ECMA definitions although we added some of
// our own operators and functionality to make it easier/faster to use in
// our environment.
//
// This plugin let you compile a JavaScript text file (string) in some form
// of byte code that works with a Forth like stack. This plugin also includes
// an interpreter of that byte code so you can run the scripts.
//
// Like in a Browser, at this point this JavaScript does not allow you to
// read and/or write to a file. It has access to the database though, with
// limits.

#include "javascript.h"
#include "plugins.h"
#include "../content/content.h"
#include <QScriptEngine>
#include <QScriptProgram>
#include <QScriptClass>
#include <QScriptClassPropertyIterator>
#include <QSharedPointer>


SNAP_PLUGIN_START(javascript, 1, 0)


/*
 * At this time we're using the Qt implementation which we assume will work
 * well enough as a JavaScript interpreter. However, this introduce a slowness
 * in that we cannot save the compiled byte code of a program. This is an
 * annoyance because we'd want to just load the byte code from the database
 * to immediately execute that. This would make things a lot faster especially
 * when each time you run you have to recompile many scripts!
 */




/** \brief Get a fixed layout name.
 *
 * The layout plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//const char *get_name(name_t name)
//{
//	switch(name) {
//	case SNAP_NAME_JAVASCRIPT_NAME:
//		return "javascript::script";
//
//	default:
//		// invalid index
//		throw snap_exception();
//
//	}
//	NOTREACHED();
//}


/** \brief Initialize the javascript plugin.
 *
 * This function is used to initialize the javascript plugin object.
 */
javascript::javascript()
	//: f_snap(NULL) -- auto-init
	//  f_dynamic_plugins() -- auto-init
{
}

/** \brief Clean up the javascript plugin.
 *
 * Ensure the layout object is clean before it is gone.
 */
javascript::~javascript()
{
}

/** \brief Initialize the javascript.
 *
 * This function terminates the initialization of the javascript plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void javascript::on_bootstrap(snap_child *snap)
{
	f_snap = snap;
}

/** \brief Get a pointer to the javascript plugin.
 *
 * This function returns an instance pointer to the javascript plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the javascript plugin.
 */
javascript *javascript::instance()
{
	return g_plugin_javascript_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString javascript::description() const
{
	return "Offer server side JavaScript support for different plugins."
			" This implementation makes use of the QtScript extension.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t javascript::do_update(int64_t last_updated)
{
	SNAP_PLUGIN_UPDATE_INIT();

	SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
	SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

	SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the javascript plugin.
 *
 * This function is the first update for the javascript plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void javascript::initial_update(int64_t variables_timestamp)
{
}

/** \brief Update the database with our javascript references.
 *
 * Send our javascript to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void javascript::content_update(int64_t variables_timestamp)
{
	content::content::instance()->add_xml("javascript");
}

/** \brief Add plugin p as a dynamic plugin.
 *
 * This function registers the specified plugin (p) as one of the
 * dynamic plugin that want to know if the user attempts to
 * access data from that plugin.
 *
 * \param[in] p  The dynamic plugin to register.
 */
void javascript::register_dynamic_plugin(javascript_dynamic_plugin *p)
{
	f_dynamic_plugins.push_back(p);
}

/** \brief Dynamic plugin object iterator.
 *
 * This class is used to iterate through the members of a dynamic plugin.
 */
class javascript_dynamic_plugin_iterator : public QScriptClassPropertyIterator
{
public:
	javascript_dynamic_plugin_iterator(javascript *js, QScriptEngine *engine, const QScriptValue& object_value, javascript_dynamic_plugin *plugin)
		: QScriptClassPropertyIterator::QScriptClassPropertyIterator(object_value),
		  f_javascript(js),
		  f_engine(engine),
		  f_pos(-1),
		  f_object(object_value),
		  f_plugin(plugin)
	{
	}

	//virtual QScriptValue::PropertyFlags flags() const;

	virtual bool hasNext() const
	{
		return f_pos + 1 < f_plugin->js_property_count();
	}

	virtual bool hasPrevious() const
	{
		return f_pos > 0;
	}

	virtual uint id() const
	{
		return f_pos;
	}

	virtual QScriptString name() const
	{
		if(f_pos < 0 || f_pos >= f_plugin->js_property_count()) {
			throw std::runtime_error("querying the name of the iterator object when the iterator pointer is out of scope");
		}
		return f_engine->toStringHandle(f_plugin->js_property_name(f_pos));
	}

	virtual void next()
	{
		if(f_pos < f_plugin->js_property_count()) {
			++f_pos;
		}
	}

	virtual void previous()
	{
		if(f_pos > -1) {
			--f_pos;
		}
	}

	virtual void toBack()
	{
		// right after the last position
		f_pos = f_plugin->js_property_count();
	}

	virtual void toFront()
	{
		// right before the first position
		f_pos = -1;
	}

	QScriptValue object() const
	{
		return f_object;
	}

private:
	javascript *				f_javascript;
	QScriptEngine *				f_engine;
	controlled_vars::mint32_t	f_pos;
	QScriptValue				f_object;
	javascript_dynamic_plugin *	f_plugin;
};

/** \brief Implement our own script class so we can dynamically get plugins values.
 *
 * This function is used to read data from the database based on the name
 * of the plugin and the name of the parameter that the user is interested
 * in. The JavaScript syntax looks like this:
 *
 * \code
 *		var n = plugins.layout.name;
 * \endcode
 *
 * In this case the layout plugin is queried for its parameter "name".
 */
class dynamic_plugin_class : public QScriptClass
{
public:
	dynamic_plugin_class(javascript *js, QScriptEngine *script_engine, javascript_dynamic_plugin *plugin)
		: QScriptClass(script_engine),
		  f_javascript(js),
		  f_plugin(plugin)
	{
//printf("Yo! got a dpc %p\n", reinterpret_cast<void*>(this));
	}

//~dynamic_plugin_class()
//{
//printf("dynamic_plugin_class destroyed... %p\n", reinterpret_cast<void*>(this));
//}

	// we don't currently support extensions
	//virtual bool supportsExtension(Extension extension) const { return false; }
	//virtual QVariant extension(Extension extension, const QVariant& argument = QVariant());

	virtual QString name() const
	{
		return dynamic_cast<plugins::plugin *>(f_plugin)->get_plugin_name();
	}

	virtual QScriptClassPropertyIterator *newIterator(const QScriptValue& object)
	{
		return new javascript_dynamic_plugin_iterator(f_javascript, engine(), object, f_plugin);
	}

	virtual QScriptValue property(const QScriptValue& object, const QScriptString& object_name, uint id)
	{
		QScriptValue result(f_plugin->js_property_get(object_name).toString());
		return result;
	}

	virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object, const QScriptString& property_name, uint id)
	{
		// at some point we may want to allow read/write/delete...
		return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::KeepExistingFlags;
	}

	virtual QScriptValue prototype() const
	{
		QScriptValue result;
		return result;
	}

	virtual QueryFlags queryProperty(const QScriptValue& object, const QScriptString& property_name, QueryFlags flags, uint * id)
	{
		return QScriptClass::HandlesReadAccess;
	}

	virtual void setProperty(QScriptValue& object, const QScriptString& property_name, uint id, const QScriptValue& value)
	{
//printf("setProperty() called... not implemented yet\n");
		throw std::runtime_error("setProperty() not implemented yet");
	}

private:
	javascript *				f_javascript;
	javascript_dynamic_plugin *	f_plugin;
};

/** \brief Plugins object iterator.
 *
 * This class is used to iterate through the list of plugins.
 */
class javascript_plugins_iterator : public QScriptClassPropertyIterator
{
public:
	javascript_plugins_iterator(javascript *js, QScriptEngine *engine, const QScriptValue& object_value)
		: QScriptClassPropertyIterator::QScriptClassPropertyIterator(object_value),
		  f_javascript(js),
		  f_engine(engine),
		  f_pos(-1),
		  f_object(object_value)
	{
//printf("javascript_plugins_iterator created!!!\n");
	}

//~javascript_plugins_iterator()
//{
//printf("javascript_plugins_iterator destroyed...\n");
//}
	//virtual QScriptValue::PropertyFlags flags() const;

	virtual bool hasNext() const
	{
		return f_pos + 1 < f_javascript->f_dynamic_plugins.size();
	}

	virtual bool hasPrevious() const
	{
		return f_pos > 0;
	}

	virtual uint id() const
	{
		return f_pos;
	}

	virtual QScriptString name() const
	{
		if(f_pos < 0 || f_pos >= f_javascript->f_dynamic_plugins.size()) {
			throw std::runtime_error("querying the name of the iterator object when the iterator pointer is out of scope");
		}
		return f_engine->toStringHandle(dynamic_cast<plugins::plugin *>(f_javascript->f_dynamic_plugins[f_pos])->get_plugin_name());
	}

	virtual void next()
	{
		if(f_pos < f_javascript->f_dynamic_plugins.size()) {
			++f_pos;
		}
	}

	virtual void previous()
	{
		if(f_pos > -1) {
			--f_pos;
		}
	}

	virtual void toBack()
	{
		// right after the last position
		f_pos = f_javascript->f_dynamic_plugins.size();
	}

	virtual void toFront()
	{
		// right before the first position
		f_pos = -1;
	}

	QScriptValue object() const
	{
		return f_object;
	}

private:
	javascript *				f_javascript;
	QScriptEngine *				f_engine;
	controlled_vars::mint32_t	f_pos;
	QScriptValue				f_object;
};

/** \brief Implement our own script class so we can dynamically get plugins values.
 *
 * This function is used to read data from the database based on the name
 * of the plugin and the name of the parameter that the user is interested
 * in. The JavaScript syntax looks like this:
 *
 * \code
 *		var n = plugins.layout.name;
 * \endcode
 *
 * In this case the layout plugin is queried for its parameter "name".
 */
class plugins_class : public QScriptClass
{
public:
	plugins_class(javascript *js, QScriptEngine *script_engine)
		: QScriptClass(script_engine),
		  f_javascript(js)
	{
//printf("plugins_class created...\n");
	}

//~plugins_class()
//{
//printf("plugins_class destroyed...\n");
//}

	// we don't currently support extensions
	//virtual bool supportsExtension(Extension extension) const { return false; }
	//virtual QVariant extension(Extension extension, const QVariant& argument = QVariant());

	virtual QString name() const
	{
		return "plugins";
	}

	virtual QScriptClassPropertyIterator *newIterator(const QScriptValue& object)
	{
		return new javascript_plugins_iterator(f_javascript, engine(), object);
	}

	virtual QScriptValue property(const QScriptValue& object, const QScriptString& object_name, uint id)
	{
		QString temp_name(object_name);
		if(f_dynamic_plugins.contains(temp_name)) {
			QScriptValue plugin_object(engine()->newObject(f_dynamic_plugins[temp_name].data()));
			return plugin_object;
		}
		int max(f_javascript->f_dynamic_plugins.size());
		for(int i(0); i < max; ++i) {
			if(dynamic_cast<plugins::plugin *>(f_javascript->f_dynamic_plugins[i])->get_plugin_name() == temp_name) {
				QSharedPointer<dynamic_plugin_class> plugin(new dynamic_plugin_class(f_javascript, engine(), f_javascript->f_dynamic_plugins[i]));
				f_dynamic_plugins[temp_name] = plugin;
				QScriptValue plugin_object(engine()->newObject(plugin.data()));
				return plugin_object;
			}
		}
		// otherwise return whatever the default is
		return QScriptClass::property(object, object_name, id);
	}

	virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object, const QScriptString& property_name, uint id)
	{
		// at some point we may want to allow read/write/delete...
		return QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::KeepExistingFlags;
	}

	virtual QScriptValue prototype() const
	{
		QScriptValue result;
		return result;
	}

	virtual QueryFlags queryProperty(const QScriptValue& object, const QScriptString& property_name, QueryFlags flags, uint * id)
	{
		return QScriptClass::HandlesReadAccess;
	}

	virtual void setProperty(QScriptValue& object, const QScriptString& property_name, uint id, const QScriptValue& value)
	{
//printf("setProperty() called... not implemented yet\n");
		throw std::runtime_error("setProperty() not implemented yet");
	}

private:
	QMap<QString, QSharedPointer<dynamic_plugin_class> >	f_dynamic_plugins;
	javascript *							f_javascript;
};

/** \brief Use this function to run a script and get the result.
 *
 * This function compiles and run the specified script and then
 * return the result.
 *
 * Note that at this time we expect that all the server side code
 * is generated by the server and thus is 100% safe to run. This
 * includes the return value is under control by the different
 * plugins using this function.
 *
 * \param[in] script  The JavaScript code to evaluate.
 *
 * \return The result in a QVariant.
 */
QVariant javascript::evaluate_script(const QString& script)
{
//printf("evaluating JS [%s]\n", script.toUtf8().data());
	QScriptProgram program(script);
	QScriptEngine engine;
	plugins_class plugins(this, &engine);
	QScriptValue plugins_object(engine.newObject(&plugins));
	engine.globalObject().setProperty("plugins", plugins_object);
//printf("object name = [%s] (%d)\n", plugins_object.scriptClass()->name().toUtf8().data(), plugins_object.isObject());
	QScriptValue value(engine.evaluate(program));
	if(engine.hasUncaughtException()) {
//QScriptValue e(engine.uncaughtException());
//printf("result = %d (%d) -> [%s]\n", engine.hasUncaughtException(), e.isError(), e.toString().toUtf8().data() );
	}
	return value.toVariant();
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4
