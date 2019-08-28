#include "python_ngcore.hpp"
#include "bitarray.hpp"


using namespace ngcore;
using namespace std;

PYBIND11_MODULE(pyngcore, m) // NOLINT
{
  ExportArray<int>(m);
  ExportArray<unsigned>(m);
  ExportArray<size_t>(m);
  ExportArray<double>(m);

  py::class_<BitArray, shared_ptr<BitArray>> (m, "BitArray")
    .def(py::init([] (size_t n) { return make_shared<BitArray>(n); }),py::arg("n"))
    .def(py::init([] (const BitArray& a) { return make_shared<BitArray>(a); } ), py::arg("ba"))
    .def(py::init([] (const vector<bool> & a)
                  {
                    auto ba = make_shared<BitArray>(a.size());
                    ba->Clear();
                    for (size_t i = 0; i < a.size(); i++)
                      if (a[i]) ba->SetBit(i);
                    return ba;
                  } ), py::arg("vec"))
    .def("__str__", &ToString<BitArray>)
    .def("__len__", &BitArray::Size)
    .def("__getitem__", [] (BitArray & self, int i)
                                         {
                                           if (i < 0 || i >= self.Size())
                                             throw py::index_error();
                                           return self.Test(i);
                                         }, py::arg("pos"), "Returns bit from given position")
    .def("__setitem__", [] (BitArray & self, int i, bool b)
                                         {
                                           if (i < 0 || i >= self.Size())
                                             throw py::index_error();
                                           if (b) self.SetBit(i); else self.Clear(i);
                                         }, py::arg("pos"), py::arg("value"), "Clear/Set bit at given position")

    .def("__setitem__", [] (BitArray & self, py::slice inds, bool b)
                                         {
                                           size_t start, step, stop, n;
                                           if (!inds.compute(self.Size(), &start, &stop, &step, &n))
                                             throw py::error_already_set();

                                           if (start == 0 && n == self.Size() && step == 1)
                                             { // base branch
                                               if (b)
                                                 self.Set();
                                               else
                                                 self.Clear();
                                             }
                                           else
                                             {
                                               if (b)
                                                 for (size_t i=0; i<n; i++, start+=step)
                                                   self.SetBit(start);
                                               else
                                                 for (size_t i=0; i<n; i++, start+=step)
                                                   self.Clear(start);
                                             }
                                         }, py::arg("inds"), py::arg("value"), "Clear/Set bit at given positions")

    .def("__setitem__", [](BitArray & self,  IntRange range, bool b)
      {
        if (b)
          for (size_t i : range)
            self.SetBit(i);
        else
          for (size_t i : range)
            self.Clear(i);
      }, py::arg("range"), py::arg("value"), "Set value for range of indices" )

    .def("NumSet", &BitArray::NumSet)
    .def("Set", [] (BitArray & self) { self.Set(); }, "Set all bits")
    .def("Set", &BitArray::SetBit, py::arg("i"), "Set bit at given position")
    .def("Clear", [] (BitArray & self) { self.Clear(); }, "Clear all bits")
    .def("Clear", [] (BitArray & self, int i)
                                   {
                                       self.Clear(i);
                                   }, py::arg("i"), "Clear bit at given position")


    .def(py::self | py::self)
    .def(py::self & py::self)
    .def(py::self |= py::self)
    .def(py::self &= py::self)
    .def(~py::self)
    ;

  py::class_<Flags>(m, "Flags")
    .def(py::init<>())
    .def("__str__", &ToString<Flags>)
    .def(py::init([](py::object & obj) {
          Flags flags;
          py::dict d(obj);          
          SetFlag (flags, "", d);
          return flags;
        }), py::arg("obj"), "Create Flags by given object")
    .def(py::pickle([] (const Flags& self)
        {
          std::stringstream str;
          self.SaveFlags(str);
          return py::make_tuple(py::cast(str.str()));
        },
        [] (py::tuple state)
        {
          string s = state[0].cast<string>();
          std::stringstream str(s);
          Flags flags;
          flags.LoadFlags(str);
          return flags;
        }
    ))
    .def("Set",[](Flags & self,const py::dict & aflags)->Flags&
    {      
      SetFlag(self, "", aflags);
      return self;
    }, py::arg("aflag"), "Set the flags by given dict")

    .def("Set",[](Flags & self, const char * akey, const py::object & value)->Flags&
    {             
        SetFlag(self, akey, value);
        return self;
    }, py::arg("akey"), py::arg("value"), "Set flag by given value.")

    .def("__getitem__", [](Flags & self, const string& name) -> py::object {

	  if(self.NumListFlagDefined(name))
	    return py::cast(self.GetNumListFlag(name));

	  if(self.StringListFlagDefined(name))
	    return py::cast(self.GetStringListFlag(name));
	 
	  if(self.NumFlagDefined(name))
	    return py::cast(*self.GetNumFlagPtr(name));
	  
	  if(self.StringFlagDefined(name))
	    return py::cast(self.GetStringFlag(name));

	  if(self.FlagsFlagDefined(name))
	    return py::cast(self.GetFlagsFlag(name));

	  return py::cast(self.GetDefineFlag(name));
      }, py::arg("name"), "Return flag by given name")
  ;
  py::implicitly_convertible<py::dict, Flags>();

  
  py::enum_<level::level_enum>(m, "LOG_LEVEL", "Logging level")
    .value("Trace", level::trace)
    .value("Debug", level::debug)
    .value("Info", level::info)
    .value("Warn", level::warn)
    .value("Error", level::err)
    .value("Critical", level::critical)
    .value("Off", level::off);

  m.def("SetLoggingLevel", &SetLoggingLevel, py::arg("level"), py::arg("logger")="",
        "Set logging level, if name is given only to the specific logger, else set the global logging level");
  m.def("AddFileSink", &AddFileSink, py::arg("filename"), py::arg("level"), py::arg("logger")="",
        "Add File sink, either only to logger specified or globally to all loggers");
  m.def("AddConsoleSink", &AddConsoleSink, py::arg("level"), py::arg("logger")="",
        "Add console output for specific logger or all if none given");
  m.def("ClearLoggingSinks", &ClearLoggingSinks, py::arg("logger")="",
        "Clear sinks of specific logger, or all if none given");
  m.def("FlushOnLoggingLevel", &FlushOnLoggingLevel, py::arg("level"), py::arg("logger")="",
        "Flush every message with level at least `level` for specific logger or all loggers if none given.");
}
