#include <tst/set.hpp>
#include <tst/check.hpp>

#include "../../../src/tml/tree_ext.hpp"

namespace{
const tst::set set("read_ext", [](auto& suite){
	suite.add("correct_extra_info_is_provided", [](){
		const auto tml = tml::read_ext(R"qwertyuiop(
			hello"world!"
			how {are you "doing"?
			}I'm okay
		)qwertyuiop");

		tst::check_eq(tml.size(), size_t(5), SL);

		auto& hello = tml[0];
		tst::check_eq(hello.value.string, std::string("hello"), SL);
		tst::check(!hello.value.info.flags.get(tml::flag::curly_braces), SL);
		tst::check(hello.value.info.flags.get(tml::flag::first_on_line), SL);
		tst::check(!hello.value.info.flags.get(tml::flag::space), SL);
		tst::check(!hello.value.info.flags.get(tml::flag::quoted), SL);
		tst::check(!hello.value.info.flags.get(tml::flag::raw), SL);
		tst::check_eq(hello.value.info.location.line, size_t(2), SL);
		tst::check_eq(hello.value.info.location.offset, size_t(4), SL);

		auto& world = tml[1];
		tst::check_eq(world.value.string, std::string("world!"), SL);
		tst::check(!world.value.info.flags.get(tml::flag::curly_braces), SL);
		tst::check(!world.value.info.flags.get(tml::flag::first_on_line), SL);
		tst::check(world.value.info.flags.get(tml::flag::quoted), SL);
		tst::check(!world.value.info.flags.get(tml::flag::space), SL);
		tst::check(!world.value.info.flags.get(tml::flag::raw), SL);
		tst::check_eq(world.value.info.location.line, size_t(2), SL);
		tst::check_eq(world.value.info.location.offset, size_t(9), SL);

		auto& how = tml[2];
		tst::check_eq(how.value.string, std::string("how"), SL);
		tst::check(how.value.info.flags.get(tml::flag::curly_braces), SL);
		tst::check(how.value.info.flags.get(tml::flag::first_on_line), SL);
		tst::check(how.value.info.flags.get(tml::flag::space), SL);
		tst::check(!how.value.info.flags.get(tml::flag::quoted), SL);
		tst::check(!how.value.info.flags.get(tml::flag::raw), SL);
		tst::check_eq(how.value.info.location.line, size_t(3), SL);
		tst::check_eq(how.value.info.location.offset, size_t(4), SL);

		{
			tst::check_eq(how.children.size(), size_t(4), SL);

			auto& are = how.children[0];
			tst::check(!are.value.info.flags.get(tml::flag::first_on_line), SL);

			auto& you = how.children[1];
			tst::check(!you.value.info.flags.get(tml::flag::first_on_line), SL);

			auto& doing = how.children[2];
			tst::check_eq(doing.value.string, std::string("doing"), SL);
			tst::check(doing.value.info.flags.get(tml::flag::space), SL);
			tst::check(doing.value.info.flags.get(tml::flag::quoted), SL);
			tst::check(!doing.value.info.flags.get(tml::flag::raw), SL);
			tst::check_eq(doing.value.info.location.line, size_t(3), SL);
			tst::check_eq(doing.value.info.location.offset, size_t(17), SL);

			auto& qm = how.children[3];
			tst::check_eq(qm.value.string, std::string("?"), SL);
			tst::check(!qm.value.info.flags.get(tml::flag::space), SL);
			tst::check(!qm.value.info.flags.get(tml::flag::quoted), SL);
			tst::check(!qm.value.info.flags.get(tml::flag::raw), SL);
			tst::check_eq(qm.value.info.location.line, size_t(3), SL);
			tst::check_eq(qm.value.info.location.offset, size_t(24), SL);
		}

		auto& im = tml[3];
		tst::check_eq(im.value.string, std::string("I'm"), SL);
		tst::check(im.value.info.flags.get(tml::flag::first_on_line), SL);
		tst::check(!im.value.info.flags.get(tml::flag::space), SL);
		tst::check(!im.value.info.flags.get(tml::flag::quoted), SL);
		tst::check(!im.value.info.flags.get(tml::flag::raw), SL);
		tst::check_eq(im.value.info.location.line, size_t(4), SL);
		tst::check_eq(im.value.info.location.offset, size_t(5), SL);

		auto& okay = tml[4];
		tst::check_eq(okay.value.string, std::string("okay"), SL);
		tst::check(!okay.value.info.flags.get(tml::flag::first_on_line), SL);
	});
});
}
